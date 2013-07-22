/* Copyright (c) 2012 Patrick Ruoff
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/opencv.hpp>
#include "videoInput/videoInput.h"
#include <boost/shared_ptr.hpp>

// ----------------------------------------------------------------------------
struct CamInfo
{
	CamInfo() : res_x(0), res_y(0), fps(0), f(1) {}

	int res_x;
	int res_y;
	int fps;
	float f;	// (focal length) / (sensor width)
};

// ----------------------------------------------------------------------------
// base class for cameras
class Camera
{
	friend class RotatedCamera;
public:
	Camera() : dt_valid(0), dt_mean(0), desired_index(0), active_index(-1), active(false) {}
	virtual ~Camera() {}

	// start/stop capturing
	virtual void start() = 0;
	virtual void stop() = 0;
	void restart() { stop(); start(); }

	void set_index(int index);
	void set_f(float f);
	void set_fps(int fps);
	void set_res(int x_res, int y_res);

	// gets a frame from the camera, dt: time since last call in seconds
	bool get_frame(float dt, cv::Mat* frame);

	// WARNING: returned references are valid as long as object
	const CamInfo& get_info() const { return cam_info; }
	const CamInfo& get_desired() const { return cam_desired; }

protected:
	// get a frame from the camera
	virtual bool _get_frame(cv::Mat* frame) = 0;

	// update the camera using cam_desired, write res and f to cam_info if successful
	virtual void _set_index() = 0;
	virtual void _set_f() = 0;
	virtual void _set_fps() = 0;
	virtual void _set_res() = 0;

	bool active;
	int desired_index;
	int active_index;
	CamInfo cam_info;
	CamInfo cam_desired;
	float dt_valid;
	float dt_mean;
};


// ----------------------------------------------------------------------------
// OpenCV camera
/*
class CVCamera : public Camera
{
public:
	CVCamera() : cap(NULL) {}
	~CVCamera() { stop(); }

	void start();
	void stop();

protected:
	bool _get_frame(cv::Mat* frame);
	void _set_index();
	void _set_f();
	void _set_fps();
	void _set_res();

	CvCapture* cap;
};
*/

// ----------------------------------------------------------------------------
// videoInput camera
class VICamera : public Camera
{
public:
	VICamera();
	~VICamera() { stop(); }

	void start();
	void stop();

protected:
	bool _get_frame(cv::Mat* frame);
	void _set_index();
	void _set_f();
	void _set_fps();
	void _set_res();

	videoInput VI;
	cv::Mat new_frame;
	unsigned char* frame_buffer;
};


// ----------------------------------------------------------------------------
// frame rotation
class FrameRotation 
{
public:
	typedef enum Rotation
	{
		CLOCKWISE = -1,
		ZERO = 0,
		COUNTER_CLOCKWISE = 1
	};
	Rotation rotation;

	cv::Mat rotate_frame(cv::Mat frame);
};

#endif //CAMERA_H
