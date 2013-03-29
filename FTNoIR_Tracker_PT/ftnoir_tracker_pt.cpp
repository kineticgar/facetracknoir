/* Copyright (c) 2012 Patrick Ruoff
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * Bugfix V171: WVR 20130508 - Invert pitch.
 */

#include "ftnoir_tracker_pt.h"
#include <QHBoxLayout>
#include <cmath>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>

using namespace std;
using namespace cv;
using namespace boost;

//#define PT_PERF_LOG	//log performance

//-----------------------------------------------------------------------------
Tracker::Tracker()
	: frame_count(0), commands(0), video_widget(NULL), tracking_valid(false)
{
	qDebug()<<"Tracker::Tracker";
	TrackerSettings settings;
	settings.load_ini();
	apply(settings);
	camera.start();
	start();
}

Tracker::~Tracker()
{
	qDebug()<<"Tracker::~Tracker";
	set_command(ABORT);
	wait();
	if (video_widget) delete video_widget;
}

void Tracker::set_command(Command command)
{
	QMutexLocker lock(&mutex);
	commands |= command;
}

void Tracker::reset_command(Command command)
{
	QMutexLocker lock(&mutex);
	commands &= ~command;
}

void Tracker::run()
{
	qDebug()<<"Tracker:: Thread started";

#ifdef PT_PERF_LOG
	QFile log_file(QCoreApplication::applicationDirPath() + "/PointTrackerPerformance.txt");
	if (!log_file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
	QTextStream log_stream(&log_file);
#endif

	time.start();
	float dt;
	bool new_frame;
	forever
	{
		{	
			QMutexLocker lock(&mutex);
			
			if (commands & ABORT) break;
			if (commands & PAUSE) continue;
			commands = 0;
			
			dt = time.elapsed() / 1000.0;
			time.restart();

			new_frame = camera.get_frame(dt, &frame);
			if (new_frame && !frame.empty())
			{
				const std::vector<cv::Vec2f>& points = point_extractor.extract_points(frame, dt, draw_frame);
				tracking_valid = point_tracker.track(points, camera.get_info().f, dt);
				frame_count++;
			}
#ifdef PT_PERF_LOG
			log_stream<<"dt: "<<dt;
			if (!frame.empty()) log_stream<<" fps: "<<camera.get_info().fps;
			log_stream<<"\n";
#endif
		}
		msleep(sleep_time);
	}

	qDebug()<<"Tracker:: Thread stopping";
}

void Tracker::apply(const TrackerSettings& settings)
{
	qDebug()<<"Tracker:: Applying settings";
	QMutexLocker lock(&mutex);
	camera.set_index(settings.cam_index);
	camera.set_res(settings.cam_res_x, settings.cam_res_y);	
	camera.set_fps(settings.cam_fps);
	camera.set_f(settings.cam_f);
	point_extractor.threshold_val = settings.threshold;
	point_extractor.min_size = settings.min_point_size;
	point_extractor.max_size = settings.max_point_size;
	point_tracker.point_model = boost::shared_ptr<PointModel>(new PointModel(settings.M01, settings.M02));
	point_tracker.dynamic_pose_resolution = settings.dyn_pose_res;
	sleep_time = settings.sleep_time;
	point_tracker.dt_reset = settings.reset_time / 1000.0;
	draw_frame = settings.video_widget;
	cam_pitch = settings.cam_pitch;

	bEnableRoll = settings.bEnableRoll;
	bEnablePitch = settings.bEnablePitch;
	bEnableYaw = settings.bEnableYaw;
	bEnableX = settings.bEnableX;
	bEnableY = settings.bEnableY;
	bEnableZ = settings.bEnableZ;

	t_MH = settings.t_MH;
	qDebug()<<"Tracker::apply ends";
}

void Tracker::reset()
{
	QMutexLocker lock(&mutex);
	point_tracker.reset();
}

void Tracker::center()
{
	point_tracker.reset();	// we also do a reset here since there is no reset shortkey yet
	QMutexLocker lock(&mutex);
	FrameTrafo X_CM_0 = point_tracker.get_pose();
	FrameTrafo X_MH(Matx33f::eye(), t_MH);
	X_CH_0 = X_CM_0 * X_MH;
}

//-----------------------------------------------------------------------------
// ITracker interface
void Tracker::Initialize(QFrame *videoframe)
{
	const int VIDEO_FRAME_WIDTH = 252;
	const int VIDEO_FRAME_HEIGHT = 189;

	qDebug("Tracker::Initialize");
	// setup video frame
	videoframe->setAttribute(Qt::WA_NativeWindow);
	videoframe->show();
	video_widget = new VideoWidget(videoframe);
	QHBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(video_widget);
	if (videoframe->layout()) delete videoframe->layout();
	videoframe->setLayout(layout);
	video_widget->resize(VIDEO_FRAME_WIDTH, VIDEO_FRAME_HEIGHT);
}

void Tracker::refreshVideo()
{	
	if (video_widget)
	{
		Mat frame_copy;
		shared_ptr< vector<Vec2f> > points;
		{
			QMutexLocker lock(&mutex);
			if (!draw_frame || frame.empty()) return;
		
			// copy the frame and points from the tracker thread
			frame_copy = frame.clone();
			points = shared_ptr< vector<Vec2f> >(new vector<Vec2f>(point_extractor.get_points()));
		}
		
		video_widget->update(frame_copy, points);
	}
}

void Tracker::StartTracker(HWND parent_window)
{
	reset_command(PAUSE);
}

void Tracker::StopTracker(bool exit)
{
	set_command(PAUSE);
}

bool Tracker::GiveHeadPoseData(THeadPoseData *data)
{
	const float rad2deg = 180.0/3.14159265;
	const float deg2rad = 1.0/rad2deg;
	{
		QMutexLocker lock(&mutex);

		if (!tracking_valid) return false;

		FrameTrafo X_CM = point_tracker.get_pose();
		FrameTrafo X_MH(Matx33f::eye(), t_MH);
		FrameTrafo X_CH = X_CM * X_MH;

		Matx33f R = X_CH.R * X_CH_0.R.t(); 
		Vec3f t   = X_CH.t - X_CH_0.t;		
		
		// correct for camera pitch
		Matx33f R_CP( 1, 0, 0,
			          0,  cos(deg2rad*cam_pitch), sin(deg2rad*cam_pitch),
					  0, -sin(deg2rad*cam_pitch), cos(deg2rad*cam_pitch));
		R = R_CP * R * R_CP.t();
		t = R_CP * t;

		// get translation(s)
		if (bEnableX) {
			data->x = t[0] / 10.0;	// convert to cm
		}
		if (bEnableY) {
			data->y = t[1] / 10.0;
		}
		if (bEnableZ) {
			data->z = t[2] / 10.0;
		}

		// translate rotation matrix from opengl (G) to roll-pitch-yaw (R) frame
		// -z -> x, y -> z, x -> -y
		Matx33f R_RG( 0, 0,-1,
			         -1, 0, 0,
					  0, 1, 0);
		R = R_RG * R * R_RG.t();

		// extract rotation angles
		float alpha, beta, gamma;
		//beta = atan2( -R(2,0), sqrt(R(0,0)*R(0,0) + R(1,0)*R(1,0)) );
		beta = atan2( -R(2,0), sqrt(R(2,1)*R(2,1) + R(2,2)*R(2,2)) );
		alpha = atan2( R(1,0), R(0,0));
		gamma = atan2( R(2,1), R(2,2));		

		if (bEnableYaw) {
			data->yaw   = rad2deg * alpha;
		}
		if (bEnablePitch) {
			data->pitch = -1.0f * rad2deg * beta;
		}
		if (bEnableRoll) {
			data->roll  = rad2deg * gamma;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
#pragma comment(linker, "/export:GetTracker=_GetTracker@0")

FTNOIR_TRACKER_BASE_EXPORT ITrackerPtr __stdcall GetTracker()
{
	return new Tracker;
}