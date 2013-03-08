/********************************************************************************
* FaceTrackNoIR		This program is a private project of the some enthusiastic	*
*					gamers from Holland, who don't like to pay much for			*
*					head-tracking.												*
*																				*
* Copyright (C) 2011	Wim Vriend (Developing)									*
*						Ron Hendriks (Researching and Testing)					*
*																				*
* Homepage																		*
*																				*
* This program is free software; you can redistribute it and/or modify it		*
* under the terms of the GNU General Public License as published by the			*
* Free Software Foundation; either version 3 of the License, or (at your		*
* option) any later version.													*
*																				*
* This program is distributed in the hope that it will be useful, but			*
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY	*
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for	*
* more details.																	*
*																				*
* You should have received a copy of the GNU General Public License along		*
* with this program; if not, see <http://www.gnu.org/licenses/>.				*
*********************************************************************************/
/*
	Modifications (last one on top):
		20130201 - WVR: Load FreeTrack 2.0 protocol instead of fake TrackIR (which is now obsolete).
		20130101 - WVR: Added "None" to filter-listbox to remove "use advanced filtering".
		20121209 - WVR: Pre-v170 DLLs will not be added to the Listbox. Initial selection was changed (made case-insensitive).
		20121014 - WVR: Added second Tracker Source for Arduino solution. The two will be mutually exclusive.
		20120929 - WVR: Disable button Filter-settings when StartTracker.
		20120918 - WVR: When AutoStart is TRUE, the program is not directly minimized any more.
						This now depends on the AutoMinimize time. Fixed the 'not showing' of the MIB.
						Also disable combo and buttons after 'Start'.
		20120917 - WVR: Added Mouse-buttons to ShortKeys.
		20120717 - WVR: FunctionConfig is now used for the Curves, instead of BezierConfig.
		20120427 - WVR: The Protocol-code was already in separate DLLs, but the ListBox was still filled �statically�. Now, a Dir() of the
						EXE-folder is done, to locate Protocol-DLLs. The Icons were also moved to the DLLs
		20120317 - WVR: The Filter and Tracker-code was moved to separate DLLs. The calling-method
						was changed accordingly. The save() and LoadSettings() functions were adapted.
						The face-tracker member-functions NotifyZeroed and refreshVideo were added, as 
						requested by Stanislaw.
		20110813 - WVR: Changed the presentation of the raw inputs: now a decimal digit will even show when '0'.
		20110404 - WVR: Migrated the FlightGear protocol to a separate DLL. The rest must follow...
		20110401 - WVR: The about-dialog was shown 'misplaced'. It was corrected.
		20110328 - WVR: Added the display for output-pose.
		20110207 - WVR: RadioButtons for 'Stop engine' added. It is now possible to choose Stop or Keep tracking.
		20110109 - WVR: Added minimizeTaskBar option added. It is now possible to choose minimized or tray.
*/
#include "FaceTrackNoIR.h"
#include "tracker.h"

//#define USE_VISAGE

//
// Setup the Main Dialog
//
FaceTrackNoIR::FaceTrackNoIR(QWidget *parent, Qt::WFlags flags) : 
QMainWindow(parent, flags),
pTrackerDialog(NULL),
pSecondTrackerDialog(NULL),
pProtocolDialog(NULL),
pFilterDialog(NULL)
{	
	cameraDetected = false;

	//
	// Initialize Widget handles, to prevent memory-access errors.
	//
	_keyboard_shortcuts = 0;
	_preferences = 0;
	_keyboard_shortcuts = 0;
	_curve_config = 0;

	tracker = 0;
//	_display = 0;
	l = 0;
	trayIcon = 0;

	setupFaceTrackNoIR();

	//
	// Read the AutoStartTracking value from the registry. If it is '1', start the Tracker and Minimize...
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	if (settings.value ( "AutoStartTracking", 0 ).toBool()) {
		startTracker();
	}

    Q_INIT_RESOURCE(PoseWidget);
	_pose_display = new GLWidget(ui.widget4logo, 0);
    _pose_display->rotateBy(0, 0, 0);

	ui.lblX->setVisible(false);
	ui.lblY->setVisible(false);
	ui.lblZ->setVisible(false);
	ui.lblRotX->setVisible(false);
	ui.lblRotY->setVisible(false);
	ui.lblRotZ->setVisible(false);

	ui.lcdNumOutputPosX->setVisible(false);
	ui.lcdNumOutputPosY->setVisible(false);
	ui.lcdNumOutputPosZ->setVisible(false);
	ui.lcdNumOutputRotX->setVisible(false);
	ui.lcdNumOutputRotY->setVisible(false);
	ui.lcdNumOutputRotZ->setVisible(false);
}

/** sets up all objects and connections to buttons */
void FaceTrackNoIR::setupFaceTrackNoIR() {
	
	ui.setupUi(this);

	ui.headPoseWidget->show();
	ui.video_frame->hide();

	// menu objects will be connected with the functions in FaceTrackNoIR class
	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(ui.btnLoad, SIGNAL(clicked()), this, SLOT(open()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(save()));
	connect(ui.actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(ui.btnSaveAs, SIGNAL(clicked()), this, SLOT(saveAs()));
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(exit()));

	connect(ui.actionPreferences, SIGNAL(triggered()), this, SLOT(showPreferences()));
	connect(ui.actionKeyboard_Shortcuts, SIGNAL(triggered()), this, SLOT(showKeyboardShortcuts()));
	connect(ui.actionCurve_Configuration, SIGNAL(triggered()), this, SLOT(showCurveConfiguration()));
	connect(ui.btnEditCurves, SIGNAL(clicked()), this, SLOT(showCurveConfiguration()));
	connect(ui.btnShortcuts, SIGNAL(clicked()), this, SLOT(showKeyboardShortcuts()));

	connect(ui.actionSupport, SIGNAL(triggered()), this, SLOT(openurl_support()));
	connect(ui.actionYour_Support, SIGNAL(triggered()), this, SLOT(openurl_donation()));
	connect(ui.btnDonate, SIGNAL(clicked()), this, SLOT(openurl_donation()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(about()));

	connect(ui.actionVideoWidget, SIGNAL(triggered()), this, SLOT(showVideoWidget()));
	connect(ui.actionHeadPoseWidget, SIGNAL(triggered()), this, SLOT(showHeadPoseWidget()));
	connect(ui.btnShowEngineControls, SIGNAL(clicked()), this, SLOT(showTrackerSettings()));
	connect(ui.btnShowSecondTrackerSettings, SIGNAL(clicked()), this, SLOT(showSecondTrackerSettings()));
	connect(ui.btnShowServerControls, SIGNAL(clicked()), this, SLOT(showServerControls()));
	connect(ui.btnShowFilterControls, SIGNAL(clicked()), this, SLOT(showFilterControls()));

	// Connect checkboxes
	connect(ui.chkInvertYaw, SIGNAL(stateChanged(int)), this, SLOT(setInvertYaw(int)));
	connect(ui.chkInvertRoll, SIGNAL(stateChanged(int)), this, SLOT(setInvertRoll(int)));
	connect(ui.chkInvertPitch, SIGNAL(stateChanged(int)), this, SLOT(setInvertPitch(int)));
	connect(ui.chkInvertX, SIGNAL(stateChanged(int)), this, SLOT(setInvertX(int)));
	connect(ui.chkInvertY, SIGNAL(stateChanged(int)), this, SLOT(setInvertY(int)));
	connect(ui.chkInvertZ, SIGNAL(stateChanged(int)), this, SLOT(setInvertZ(int)));

	// button methods connect with methods in this class
	connect(ui.btnStartTracker, SIGNAL(clicked()), this, SLOT(startTracker()));
	connect(ui.btnStopTracker, SIGNAL(clicked()), this, SLOT(stopTracker()));

	// Connect slider for smoothing
	connect(ui.slideSmoothing, SIGNAL(valueChanged(int)), this, SLOT(setSmoothing(int)));

	//read the camera-name, using DirectShow
	GetCameraNameDX();
	
	//Create the system-tray and connect the events for that.
	createIconGroupBox();
	createActions();
	createTrayIcon();

	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

	//Load the tracker-settings, from the INI-file
	loadSettings();
	trayIcon->show();

	connect(ui.iconcomboProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolSelected(int)));
	connect(ui.iconcomboProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(profileSelected(int)));
	connect(ui.iconcomboTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	connect(ui.iconcomboFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(filterSelected(int)));

	//Setup the timer for automatically minimizing after StartTracker.
	timMinimizeFTN = new QTimer(this);
    connect(timMinimizeFTN, SIGNAL(timeout()), this, SLOT(showMinimized()));

	//Setup the timer for showing the headpose.
	timUpdateHeadPose = new QTimer(this);
    connect(timUpdateHeadPose, SIGNAL(timeout()), this, SLOT(showHeadPose()));
	ui.txtTracking->setVisible(false);
	ui.txtAxisReverse->setVisible(false);
	ui.gameName->setText("");
}

/** destructor stops the engine and quits the faceapi **/
FaceTrackNoIR::~FaceTrackNoIR() {

	//
	// Stop the tracker, by simulating a button-push
	//
	stopTracker();

	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard, QMessageBox::Discard );

		switch (ret) {
			case QMessageBox::Save:
				saveAs();
				break;
			case QMessageBox::Discard:
				// Don't Save was clicked
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
}

//
// Get the ProgramName from a connected game and display it.
//
void FaceTrackNoIR::getGameProgramName() {
	if ( tracker != NULL ) {
		ui.gameName->setText( tracker->getGameProgramName() );
	}
}

//
// Update the Settings, after a value has changed. This way, the Tracker does not have to re-start.
//
void FaceTrackNoIR::updateSettings() {
	if ( tracker != NULL ) {
		tracker->loadSettings();
	}
}

//
// Get a pointer to the video-widget, to use in the DLL
//
QFrame *FaceTrackNoIR::getVideoWidget() {
	return ui.video_frame;
}

//
// Return the name of the Protocol-DLL
//
QString FaceTrackNoIR::getCurrentProtocolName()
{
	if (ui.iconcomboProtocol->currentIndex() < 0) {
		return QString("");
	}
	else {
		return protocolFileList.at(ui.iconcomboProtocol->currentIndex());
	}
}

//
// Return the name of the Filter-DLL
//
QString FaceTrackNoIR::getCurrentFilterName()
{
	qDebug() << "getCurrentFilterName says: " << ui.iconcomboFilter->currentIndex();
	if (ui.iconcomboFilter->currentIndex() <= 0) {
		return QString("None");
	}
	else {
		return filterFileList.at(ui.iconcomboFilter->currentIndex() - 1 );
	}
}

//
// Return the name of the Tracker-DLL
//
QString FaceTrackNoIR::getCurrentTrackerName()
{
	if (ui.iconcomboTrackerSource->currentIndex() < 0) {
		return QString("");
	}
	else {
		qDebug() << "FaceTrackNoIR::getCurrentTrackerName libName = " << trackerFileList.at(ui.iconcomboTrackerSource->currentIndex());
		return trackerFileList.at(ui.iconcomboTrackerSource->currentIndex());
	}
}

//
// Return the name of the second Tracker-DLL
//
QString FaceTrackNoIR::getSecondTrackerName()
{
	if (ui.cbxSecondTrackerSource->currentIndex() <= 0) {
		return QString("None");
	}
	else {
		return trackerFileList.at(ui.cbxSecondTrackerSource->currentIndex() - 1 );
	}
}

/** read the name of the first video-capturing device at start up **/
/** FaceAPI can only use this first one... **/
void FaceTrackNoIR::GetCameraNameDX() {
	
////	ui.widget->setCameraName("No video-capturing device was found in your system: check if it's connected!");

	ui.cameraName->setText("No video-capturing device was found in your system: check if it's connected!");

	// Create the System Device Enumerator.
	HRESULT hr;
	ICreateDevEnum *pSysDevEnum = NULL;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if (FAILED(hr))
	{
		qDebug() << "GetWDM says: CoCreateInstance Failed!";
		return;
	}

	qDebug() << "GetWDM says: CoCreateInstance succeeded!";
	
	// Obtain a class enumerator for the video compressor category.
	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

	if (hr == S_OK) {
		qDebug() << "GetWDM says: CreateClassEnumerator succeeded!";

		// Enumerate the monikers.
		IMoniker *pMoniker = NULL;
		ULONG cFetched;
		if (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) {
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
			if (SUCCEEDED(hr))	{
				// To retrieve the filter's friendly name, do the following:
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr))
				{
					// Display the name in your UI somehow.
					QString str((QChar*)varName.bstrVal, wcslen(varName.bstrVal));
					qDebug() << "GetWDM says: Moniker found:" << str;
					ui.cameraName->setText(str);
				}
				VariantClear(&varName);

				////// To create an instance of the filter, do the following:
				////IBaseFilter *pFilter;
				////hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,
				////	(void**)&pFilter);
				// Now add the filter to the graph. 
				//Remember to release pFilter later.
				pPropBag->Release();
			}
			pMoniker->Release();
		}
		pEnumCat->Release();
	}
	pSysDevEnum->Release();

}

//
// Open an INI-file with the QFileDialog
// If succesfull, the settings in it will be read
//
void FaceTrackNoIR::open() {
	 QFileDialog::Options options;
	 QFileDialog::FileMode mode;

     options |= QFileDialog::DontUseNativeDialog;
	 mode = QFileDialog::ExistingFile;
     QString selectedFilter;
	 QStringList fileNames = QFileDialog::getOpenFileNames(
								this,
                                 tr("Select one FTNoir settings file"),
								 QCoreApplication::applicationDirPath() + "/Settings",
                                 tr("Settings file (*.ini);;All Files (*)"));

	//
	// If a file was selected, save it's name and read it's contents.
	//
	if (! fileNames.isEmpty() ) {
		QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
		settings.setValue ("SettingsFile", fileNames.at(0));
		loadSettings();
	}
}

//
// Save the current Settings to the currently 'active' INI-file.
//
void FaceTrackNoIR::save() {

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	iniFile.beginGroup ( "Tracking" );
	iniFile.setValue ( "Smooth", ui.slideSmoothing->value() );
	iniFile.setValue ( "invertYaw", ui.chkInvertYaw->isChecked() );
	iniFile.setValue ( "invertPitch", ui.chkInvertPitch->isChecked() );
	iniFile.setValue ( "invertRoll", ui.chkInvertRoll->isChecked() );
	iniFile.setValue ( "invertX", ui.chkInvertX->isChecked() );
	iniFile.setValue ( "invertY", ui.chkInvertY->isChecked() );
	iniFile.setValue ( "invertZ", ui.chkInvertZ->isChecked() );
	iniFile.endGroup ();

	iniFile.beginGroup ( "GameProtocol" );
	iniFile.setValue ( "Selection", ui.iconcomboProtocol->currentIndex() );
	iniFile.setValue ( "DLL", getCurrentProtocolName() );
	iniFile.endGroup ();

	iniFile.beginGroup ( "TrackerSource" );
	iniFile.setValue ( "Selection", ui.iconcomboTrackerSource->currentIndex() );
	iniFile.setValue ( "DLL", getCurrentTrackerName() );
	iniFile.setValue ( "2ndDLL", getSecondTrackerName() );
	iniFile.endGroup ();

	//
	// Save the name of the filter in the INI-file.
	//
	iniFile.beginGroup ( "Filter" );
	iniFile.setValue ( "DLL", getCurrentFilterName() );
	iniFile.endGroup ();

	settingsDirty = false;
}

//
// Get the new name of the INI-file and save the settings to it.
//
// The user may choose to overwrite an existing file. This will be deleted, before copying the current file to it.
//
void FaceTrackNoIR::saveAs()
{
	//
	// Get the current filename of the INI-file.
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	QString oldFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();

	//
	// Get the new filename of the INI-file.
	//
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"),
													oldFile,
//													QCoreApplication::applicationDirPath() + "/Settings",
													tr("Settings file (*.ini);;All Files (*)"));
	if (!fileName.isEmpty()) {

		//
		// Remove the file, if it already exists.
		//
		QFileInfo newFileInfo ( fileName );
		if ((newFileInfo.exists()) && (oldFile != fileName)) {
			QFile newFileFile ( fileName );
			newFileFile.remove();
		}

		//
		// Copy the current INI-file to the new name.
		//
		QFileInfo oldFileInfo ( oldFile );
		if (oldFileInfo.exists()) {
			QFile oldFileFile ( oldFile );
			oldFileFile.copy( fileName );
		}

		//
		// Write the new name to the Registry and save the other INI-values.
		//
		settings.setValue ("SettingsFile", fileName);
		save();

		//
		// Reload the settings, to get the GUI right again...
		//
		loadSettings();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void FaceTrackNoIR::loadSettings() {

	qDebug() << "loadSettings says: Starting ";
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	//
	// Put the filename in the window-title.
	//
    QFileInfo pathInfo ( currentFile );
    setWindowTitle ( "FaceTrackNoIR (1.7.2) - " + pathInfo.fileName() );

	//
	// Get a List of all the INI-files in the (currently active) Settings-folder.
	//
	QDir settingsDir( pathInfo.dir() );
    QStringList filters;
    filters << "*.ini";
	iniFileList.clear();
	iniFileList = settingsDir.entryList( filters, QDir::Files, QDir::Name );
	
	//
	// Add strings to the Listbox.
	//
	disconnect(ui.iconcomboProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(profileSelected(int)));
	ui.iconcomboProfile->clear();
	for ( int i = 0; i < iniFileList.size(); i++) {
		ui.iconcomboProfile->addItem(QIcon(":/images/Settings16.png"), iniFileList.at(i));
		if (iniFileList.at(i) == pathInfo.fileName()) {
			ui.iconcomboProfile->setItemIcon(i, QIcon(":/images/SettingsOpen16.png"));
			ui.iconcomboProfile->setCurrentIndex( i );
		}
	}
	connect(ui.iconcomboProfile, SIGNAL(currentIndexChanged(int)), this, SLOT(profileSelected(int)));

	qDebug() << "loadSettings says: iniFile = " << currentFile;

	iniFile.beginGroup ( "Tracking" );
	ui.slideSmoothing->setValue (iniFile.value ( "Smooth", 10 ).toInt());
	ui.chkInvertYaw->setChecked (iniFile.value ( "invertYaw", 0 ).toBool());
	ui.chkInvertPitch->setChecked (iniFile.value ( "invertPitch", 0 ).toBool());
	ui.chkInvertRoll->setChecked (iniFile.value ( "invertRoll", 0 ).toBool());
	ui.chkInvertX->setChecked (iniFile.value ( "invertX", 0 ).toBool());
	ui.chkInvertY->setChecked (iniFile.value ( "invertY", 0 ).toBool());
	ui.chkInvertZ->setChecked (iniFile.value ( "invertZ", 0 ).toBool());
	iniFile.endGroup ();

	//
	// Read the currently selected Protocol from the INI-file.
	// If the setting "DLL" isn't found (pre-1.7 version of INI), then the setting 'Selection' is evaluated.
	//
	iniFile.beginGroup ( "GameProtocol" );

	QString selectedProtocolName = iniFile.value ( "DLL", "" ).toString();
	qDebug() << "loadSettings says: selectedProtocolName = " << selectedProtocolName;

	if (selectedProtocolName.length() == 0) {
		int index = iniFile.value ( "Selection", 0 ).toInt();
		switch ( index ) {
			case TRACKIR:
			case FREE_TRACK:
				selectedProtocolName = QString("FTNoIR_Protocol_FT.dll");
				break;

			case SIMCONNECT:
				selectedProtocolName = QString("FTNoIR_Protocol_SC.dll");
				break;

			case PPJOY:
				selectedProtocolName = QString("FTNoIR_Protocol_PPJOY.dll");
				break;

			case FSUIPC:
				selectedProtocolName = QString("FTNoIR_Protocol_FSUIPC.dll");
				break;

			case FLIGHTGEAR:
				selectedProtocolName = QString("FTNoIR_Protocol_FG.dll");
				break;

			case FTNOIR:
				selectedProtocolName = QString("FTNoIR_Protocol_FTN.dll");
				break;

			case MOUSE:
				selectedProtocolName = QString("FTNoIR_Protocol_MOUSE.dll");
				break;

			default:
				selectedProtocolName = QString("FTNoIR_Protocol_MOUSE.dll");
				break;
		}
	}
	iniFile.endGroup ();

	//
	// Find the Index of the DLL and set the selection.
	//
	for ( int i = 0; i < protocolFileList.size(); i++) {
		if (protocolFileList.at(i).compare( selectedProtocolName, Qt::CaseInsensitive ) == 0) {
			ui.iconcomboProtocol->setCurrentIndex( i );
			break;
		}
	}

	//
	// Read the currently selected Tracker from the INI-file.
	// If the setting "DLL" isn't found (pre-1.7 version), then the setting 'Selection' is evaluated.
	//
	iniFile.beginGroup ( "TrackerSource" );
	QString selectedTrackerName = iniFile.value ( "DLL", "" ).toString();
	qDebug() << "loadSettings says: selectedTrackerName = " << selectedTrackerName;
	if (selectedTrackerName.length() == 0) {
		int index = iniFile.value ( "Selection", 0 ).toInt();
		switch ( index ) {
			case 0:										// Face API
				selectedTrackerName = "FTNoIR_Tracker_SM.dll";
				break;
			case 1:										// FTNoir server
				selectedTrackerName = "FTNoIR_Tracker_UDP.dll";
				break;
			default:
				selectedTrackerName = "FTNoIR_Tracker_SM.dll";
				break;
		}
	}
	QString secondTrackerName = iniFile.value ( "2ndDLL", "None" ).toString();
	qDebug() << "loadSettings says: secondTrackerName = " << secondTrackerName;

	iniFile.endGroup ();

	disconnect(ui.iconcomboTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	disconnect(ui.cbxSecondTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	for ( int i = 0; i < trackerFileList.size(); i++) {
		if (trackerFileList.at(i).compare( selectedTrackerName, Qt::CaseInsensitive ) == 0) {
			ui.iconcomboTrackerSource->setCurrentIndex( i );
		}
		if (trackerFileList.at(i).compare( secondTrackerName, Qt::CaseInsensitive ) == 0) {
			ui.cbxSecondTrackerSource->setCurrentIndex( i + 1 );		// The first value = "None", so add 1
		}
	}
	connect(ui.iconcomboTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	connect(ui.cbxSecondTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));

	//
	// Read the currently selected Filter from the INI-file.
	//
	iniFile.beginGroup ( "Filter" );
	QString selectedFilterName = iniFile.value ( "DLL", "FTNoIR_Filter_EWMA2.dll" ).toString();
	qDebug() << "createIconGroupBox says: selectedFilterName = " << selectedFilterName;
	iniFile.endGroup ();

	//
	// Find the Index of the DLL and set the selection.
	//
	for ( int i = 0; i < filterFileList.size(); i++) {
		if (filterFileList.at(i).compare( selectedFilterName, Qt::CaseInsensitive ) == 0) {
			ui.iconcomboFilter->setCurrentIndex( i + 1 );		// The first value = "None", so add 1
			break;
		}
	}

	settingsDirty = false;
}

/** show support page in web-browser **/
void FaceTrackNoIR::openurl_support() {
	QDesktopServices::openUrl(QUrl("http://facetracknoir.sourceforge.net/manual/manual.htm", QUrl::TolerantMode));
}

/** show donations page in web-browser **/
void FaceTrackNoIR::openurl_donation() {
	QDesktopServices::openUrl(QUrl("http://facetracknoir.sourceforge.net/information_links/donate.htm", QUrl::TolerantMode));
}


/** show about dialog **/
void FaceTrackNoIR::about() {

	QPoint offsetpos(100, 100);
	aboutDialog.move(this->pos() + offsetpos);
	aboutDialog.show();

	/** ABOUT DIALOG **/
	aboutDialog.setBaseSize(270, 440);

	aboutDialog.setMaximumWidth(270);
	aboutDialog.setMaximumHeight(440);

	aboutDialog.setMinimumWidth(270);
	aboutDialog.setMinimumHeight(440);
	aboutDialog.setStyleSheet("background:#fff url(:/UIElements/aboutFaceTrackNoIR.png) no-repeat;");
}

/** start tracking the face **/
void FaceTrackNoIR::startTracker( ) {	

	// 
	// Disable buttons
	//
	ui.iconcomboProfile->setEnabled ( false );
	ui.btnLoad->setEnabled ( false );
	ui.btnSave->setEnabled ( false );
	ui.btnSaveAs->setEnabled ( false );
	ui.btnShowFilterControls->setEnabled ( false );

	//
	// Create the Tracker and setup
	//
	tracker = new Tracker ( this );

	//
	// Setup the Tracker and send the settings.
	// This is necessary, because the events are only triggered 'on change'
	//
	tracker->setup();
	tracker->setSmoothing ( ui.slideSmoothing->value() );
	tracker->setInvertYaw (ui.chkInvertYaw->isChecked() );
	tracker->setInvertPitch (ui.chkInvertPitch->isChecked() );
	tracker->setInvertRoll (ui.chkInvertRoll->isChecked() );
	tracker->setInvertX (ui.chkInvertX->isChecked() );
	tracker->setInvertY (ui.chkInvertY->isChecked() );
	tracker->setInvertZ (ui.chkInvertZ->isChecked() );

	tracker->start( QThread::TimeCriticalPriority );

	//
	// Register the Tracker instance with the Tracker Dialog (if open)
	//
	if (pTrackerDialog) {
		pTrackerDialog->registerTracker( tracker->getTrackerPtr() );
	}

	ui.headPoseWidget->show();

	// 
	ui.btnStartTracker->setEnabled ( false );
	ui.btnStopTracker->setEnabled ( true );

	// Enable/disable Protocol-server Settings
	ui.iconcomboTrackerSource->setEnabled ( false );
	ui.cbxSecondTrackerSource->setEnabled ( false );
	ui.iconcomboProtocol->setEnabled ( false );
//	ui.btnShowServerControls->setEnabled ( false );
	ui.iconcomboFilter->setEnabled ( false );

	//
	// Update the camera-name, FaceAPI can only use the 1st one found!
	//
	GetCameraNameDX();


	//
	// Get the TimeOut value for minimizing FaceTrackNoIR
	// Only start the Timer if value > 0
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	int timevalue = settings.value ( "AutoMinimizeTime", 0 ).toInt() * 1000;
	if (timevalue > 0) {

		bool minimizeTaskBar = settings.value ( "MinimizeTaskBar", 1 ).toBool();
		if (minimizeTaskBar) {
			connect(timMinimizeFTN, SIGNAL(timeout()), this, SLOT(showMinimized()));
		}
		else {
			connect(timMinimizeFTN, SIGNAL(timeout()), this, SLOT(hide()));
		}

		timMinimizeFTN->setSingleShot( true );
		timMinimizeFTN->start(timevalue);
	}

	//
	// Start the timer to update the head-pose (digits and 'man in black')
	//
	timUpdateHeadPose->start(50);

	ui.lblX->setVisible(true);
	ui.lblY->setVisible(true);
	ui.lblZ->setVisible(true);
	ui.lblRotX->setVisible(true);
	ui.lblRotY->setVisible(true);
	ui.lblRotZ->setVisible(true);

	ui.lcdNumOutputPosX->setVisible(true);
	ui.lcdNumOutputPosY->setVisible(true);
	ui.lcdNumOutputPosZ->setVisible(true);
	ui.lcdNumOutputRotX->setVisible(true);
	ui.lcdNumOutputRotY->setVisible(true);
	ui.lcdNumOutputRotZ->setVisible(true);
}

/** stop tracking the face **/
void FaceTrackNoIR::stopTracker( ) {	

	//
	// Stop displaying the head-pose.
	//
	timUpdateHeadPose->stop();
    _pose_display->rotateBy(0, 0, 0);


	ui.lblX->setVisible(false);
	ui.lblY->setVisible(false);
	ui.lblZ->setVisible(false);
	ui.lblRotX->setVisible(false);
	ui.lblRotY->setVisible(false);
	ui.lblRotZ->setVisible(false);

	ui.lcdNumOutputPosX->setVisible(false);
	ui.lcdNumOutputPosY->setVisible(false);
	ui.lcdNumOutputPosZ->setVisible(false);
	ui.lcdNumOutputRotX->setVisible(false);
	ui.lcdNumOutputRotY->setVisible(false);
	ui.lcdNumOutputRotZ->setVisible(false);
	ui.txtTracking->setVisible(false);
	ui.txtAxisReverse->setVisible(false);

	//
	// UnRegister the Tracker instance with the Tracker Dialog (if open)
	//
	if (pTrackerDialog) {
		pTrackerDialog->unRegisterTracker();
	}
	if (pProtocolDialog) {
		pProtocolDialog->unRegisterProtocol();
	}

	//
	// Delete the tracker (after stopping things and all).
	//
	if ( tracker ) {
		qDebug() << "stopTracker says: Deleting tracker!";
		delete tracker;
		qDebug() << "stopTracker says: Tracker deleted!";
		tracker = 0;
	}
	ui.btnStartTracker->setEnabled ( true );
	ui.btnStopTracker->setEnabled ( false );
//	ui.btnShowEngineControls->setEnabled ( false );
	ui.iconcomboProtocol->setEnabled ( true );
	ui.iconcomboTrackerSource->setEnabled ( true );
	ui.cbxSecondTrackerSource->setEnabled ( true );
	ui.iconcomboFilter->setEnabled ( true );

	// Enable/disable Protocol-server Settings
	ui.btnShowServerControls->setEnabled ( true );
	ui.video_frame->hide();

	// 
	ui.iconcomboProfile->setEnabled ( true );
	ui.btnLoad->setEnabled ( true );
	ui.btnSave->setEnabled ( true );
	ui.btnSaveAs->setEnabled ( true );
	ui.btnShowFilterControls->setEnabled ( true );

	//
	// Stop the timer, so it won't go off again...
	//
	timMinimizeFTN->stop();

}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertYaw( int invert ) {
	Tracker::setInvertYaw ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertPitch( int invert ) {
	Tracker::setInvertPitch ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertRoll( int invert ) {
	Tracker::setInvertRoll ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertX( int invert ) {
	Tracker::setInvertX ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertY( int invert ) {
	Tracker::setInvertY ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** set the invert from the checkbox **/
void FaceTrackNoIR::setInvertZ( int invert ) {
	Tracker::setInvertZ ( (invert != 0)?true:false );
	settingsDirty = true;
}

/** Show the headpose in the widget (triggered by timer) **/
void FaceTrackNoIR::showHeadPose() {
THeadPoseData newdata;

	ui.lblX->setVisible(true);
	ui.lblY->setVisible(true);
	ui.lblZ->setVisible(true);
	ui.lblRotX->setVisible(true);
	ui.lblRotY->setVisible(true);
	ui.lblRotZ->setVisible(true);

	ui.lcdNumOutputPosX->setVisible(true);
	ui.lcdNumOutputPosY->setVisible(true);
	ui.lcdNumOutputPosZ->setVisible(true);
	ui.lcdNumOutputRotX->setVisible(true);
	ui.lcdNumOutputRotY->setVisible(true);
	ui.lcdNumOutputRotZ->setVisible(true);

	//
	// Get the pose and also display it.
	// Updating the pose from within the Tracker-class caused crashes...
	//
	Tracker::getHeadPose(&newdata);
	ui.lcdNumX->display(QString("%1").arg(newdata.x, 0, 'f', 1));
	ui.lcdNumY->display(QString("%1").arg(newdata.y, 0, 'f', 1));
	ui.lcdNumZ->display(QString("%1").arg(newdata.z, 0, 'f', 1));

	ui.lcdNumRotX->display(QString("%1").arg(newdata.yaw, 0, 'f', 1));
	ui.lcdNumRotY->display(QString("%1").arg(newdata.pitch, 0, 'f', 1));
	ui.lcdNumRotZ->display(QString("%1").arg(newdata.roll, 0, 'f', 1));

	ui.txtTracking->setVisible(Tracker::getTrackingActive());
	ui.txtAxisReverse->setVisible(Tracker::getAxisReverse());

	//
	// Get the output-pose and also display it.
	//
	if (_pose_display) {
		Tracker::getOutputHeadPose(&newdata);
		_pose_display->rotateBy(newdata.pitch, newdata.yaw, newdata.roll);

		ui.lcdNumOutputPosX->display(QString("%1").arg(newdata.x, 0, 'f', 1));
		ui.lcdNumOutputPosY->display(QString("%1").arg(newdata.y, 0, 'f', 1));
		ui.lcdNumOutputPosZ->display(QString("%1").arg(newdata.z, 0, 'f', 1));

		ui.lcdNumOutputRotX->display(QString("%1").arg(newdata.yaw, 0, 'f', 1));
		ui.lcdNumOutputRotY->display(QString("%1").arg(newdata.pitch, 0, 'f', 1));
		ui.lcdNumOutputRotZ->display(QString("%1").arg(newdata.roll, 0, 'f', 1));
	}

	//
	// Update the video-widget.
	// Requested by Stanislaw
	//
	if (tracker) {
		ITracker * theTracker =	tracker->getTrackerPtr();
		if (theTracker) {
			theTracker->refreshVideo();
		}
	}
//	Tracker::doRefreshVideo();

	if (_curve_config) {
		_curve_config->update();
	}

}

/** set the smoothing from the slider **/
void FaceTrackNoIR::setSmoothing( int smooth ) {
	
	//
	// Pass the smoothing setting, if the Tracker exists.
	//
	if ( tracker ) {
		tracker->setSmoothing ( smooth );
		settingsDirty = true;
	}
}


/** toggles Video Widget **/
void FaceTrackNoIR::showVideoWidget() {
	if(ui.video_frame->isHidden())
		ui.video_frame->show();
	else
		ui.video_frame->hide();
}

/** toggles Video Widget **/
void FaceTrackNoIR::showHeadPoseWidget() {
	if(ui.headPoseWidget->isHidden())
		ui.headPoseWidget->show();
	else
		ui.headPoseWidget->hide();
}

/** toggles Engine Controls Dialog **/
void FaceTrackNoIR::showTrackerSettings() {
importGetTrackerDialog getIT;
QLibrary *trackerLib;
QString libName;

	qDebug() << "FaceTrackNoIR::showTrackerSettings started.";

	//
	// Delete the existing QDialog
	//
	if (pTrackerDialog) {
		delete pTrackerDialog;
		pTrackerDialog = NULL;
	}

	// Show the appropriate Tracker Settings
	libName.clear();
	libName = getCurrentTrackerName();

	//
	// Load the Server-settings dialog (if any) and show it.
	//
	if (!libName.isEmpty()) {
		trackerLib = new QLibrary(libName);

//		qDebug() << "FaceTrackNoIR::showTrackerSettings Loaded trackerLib." << trackerLib;

		getIT = (importGetTrackerDialog) trackerLib->resolve("GetTrackerDialog");

//		qDebug() << "FaceTrackNoIR::showTrackerSettings resolved." << getIT;

		if (getIT) {
			ITrackerDialog *ptrXyz(getIT());
			if (ptrXyz)
			{
				pTrackerDialog = ptrXyz;
				pTrackerDialog->Initialize( this );
//				qDebug() << "FaceTrackNoIR::showTrackerSettings GetTrackerDialog Function Resolved!";
				if (tracker) {
					pTrackerDialog->registerTracker( tracker->getTrackerPtr() );
//					qDebug() << "FaceTrackNoIR::showTrackerSettings RegisterTracker Function Executed";
				}
			}
		}
		else {
			QMessageBox::warning(0,"FaceTrackNoIR Error", "DLL not loaded",QMessageBox::Ok,QMessageBox::NoButton);
		}
	}

}

// Show the Settings dialog for the secondary Tracker
void FaceTrackNoIR::showSecondTrackerSettings() {
importGetTrackerDialog getIT;
QLibrary *trackerLib;
QString libName;

	qDebug() << "FaceTrackNoIR::showSecondTrackerSettings started.";

	//
	// Delete the existing QDialog
	//
	if (pSecondTrackerDialog) {
		delete pSecondTrackerDialog;
		pSecondTrackerDialog = NULL;
	}

	// Show the appropriate Tracker Settings
	libName.clear();
	libName = getSecondTrackerName();

	//
	// Load the Server-settings dialog (if any) and show it.
	//
	if ((!libName.isEmpty()) && (libName != "None")) {
		trackerLib = new QLibrary(libName);

//		qDebug() << "FaceTrackNoIR::showTrackerSettings Loaded trackerLib." << trackerLib;

		getIT = (importGetTrackerDialog) trackerLib->resolve("GetTrackerDialog");

//		qDebug() << "FaceTrackNoIR::showTrackerSettings resolved." << getIT;

		if (getIT) {
			ITrackerDialog *ptrXyz(getIT());
			if (ptrXyz)
			{
				pSecondTrackerDialog = ptrXyz;
				pSecondTrackerDialog->Initialize( this );
//				qDebug() << "FaceTrackNoIR::showTrackerSettings GetTrackerDialog Function Resolved!";
				if (tracker) {
					pSecondTrackerDialog->registerTracker( tracker->getSecondTrackerPtr() );
//					qDebug() << "FaceTrackNoIR::showTrackerSettings RegisterTracker Function Executed";
				}
			}
		}
		else {
			QMessageBox::warning(0,"FaceTrackNoIR Error", "DLL not loaded",QMessageBox::Ok,QMessageBox::NoButton);
		}
	}

}

/** toggles Server Controls Dialog **/
void FaceTrackNoIR::showServerControls() {
importGetProtocolDialog getIT;
QLibrary *protocolLib;
QString libName;

	//
	// Delete the existing QDialog
	//
	if (pProtocolDialog) {
		delete pProtocolDialog;
	}

	// Show the appropriate Protocol-server Settings
	libName.clear();
	libName = getCurrentProtocolName();

	//
	// Load the Server-settings dialog (if any) and show it.
	//
	if (!libName.isEmpty()) {
		protocolLib = new QLibrary(libName);

		getIT = (importGetProtocolDialog) protocolLib->resolve("GetProtocolDialog");
		if (getIT) {
			IProtocolDialogPtr ptrXyz(getIT());
			if (ptrXyz)
			{
				pProtocolDialog = ptrXyz;
				pProtocolDialog->Initialize( this );
				if (tracker) {
					pProtocolDialog->registerProtocol( tracker->getProtocolPtr() );
					qDebug() << "FaceTrackNoIR::showServerControls RegisterProtocol Function Executed";
				}
				qDebug() << "FaceTrackNoIR::showServerControls GetProtocolDialog Function Resolved!";
			}
			else {
				qDebug() << "FaceTrackNoIR::showServerControls Function NOT Resolved!";
			}	
		}
		else {
			QMessageBox::warning(0,"FaceTrackNoIR Error", "DLL not loaded",QMessageBox::Ok,QMessageBox::NoButton);
		}
	}
}

/** toggles Filter Controls Dialog **/
void FaceTrackNoIR::showFilterControls() {
importGetFilterDialog getIT;
QLibrary *filterLib;
QString libName;

	//
	// Delete the existing QDialog
	//
	if (pFilterDialog) {
		delete pFilterDialog;
		pFilterDialog = NULL;
	}

	// Get the currently selected Filter
	libName.clear();
	libName = getCurrentFilterName();

	//
	// Load the Filter-settings dialog (if any) and show it.
	//
	if (!libName.isEmpty()) {
		filterLib = new QLibrary(libName);

		getIT = (importGetFilterDialog) filterLib->resolve("GetFilterDialog");
		if (getIT) {
			IFilterDialogPtr ptrXyz(getIT());
			if (ptrXyz)
			{
				pFilterDialog = ptrXyz;
				pFilterDialog->Initialize( this, Tracker::getFilterPtr() );
				qDebug() << "FaceTrackNoIR::showFilterControls GetFilterDialog Function Resolved!";
			}
			else {
				qDebug() << "FaceTrackNoIR::showFilterControls Function NOT Resolved!";
			}	
		}
		else {
			QMessageBox::warning(0,"FaceTrackNoIR Error", "DLL not loaded",QMessageBox::Ok,QMessageBox::NoButton);
		}
	}
}

/** toggles FaceTrackNoIR Preferences Dialog **/
void FaceTrackNoIR::showPreferences() {

	// Create if new
	if (!_preferences)
    {
        _preferences = new PreferencesDialog( this, this, Qt::Dialog );
    }

	// Show if already created
	if (_preferences) {
		_preferences->show();
		_preferences->raise();
	}
}

/** toggles Keyboard Shortcut Dialog **/
void FaceTrackNoIR::showKeyboardShortcuts() {

	// Create if new
	if (!_keyboard_shortcuts)
    {
        _keyboard_shortcuts = new KeyboardShortcutDialog( this, this, Qt::Dialog );
    }

	// Show if already created
	if (_keyboard_shortcuts) {
		_keyboard_shortcuts->show();
		_keyboard_shortcuts->raise();
	}
}

/** toggles Curve Configuration Dialog **/
void FaceTrackNoIR::showCurveConfiguration() {

	// Create if new
	if (!_curve_config)
    {
        _curve_config = new CurveConfigurationDialog( this, this, Qt::Dialog );
    }

	// Show if already created
	if (_curve_config) {
		_curve_config->show();
		_curve_config->raise();
	}
}

/** exit application **/
void FaceTrackNoIR::exit() {
	QCoreApplication::exit(0);
}

//
// Setup the icons for the comboBoxes
//
void FaceTrackNoIR::createIconGroupBox()
{
importGetProtocolDll getProtocol;
IProtocolDllPtr pProtocolDll;			// Pointer to Protocol info instance (in DLL)
importGetFilterDll getFilter;
IFilterDllPtr pFilterDll;				// Pointer to Filter info instance (in DLL)
importGetTrackerDll getTracker;
ITrackerDll *pTrackerDll;				// Pointer to Tracker info instance (in DLL)
QStringList listDLLs;					// List of specific DLLs

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	//
	// Get a List of all the Protocol-DLL-files in the Program-folder.
	//
	QDir settingsDir( QCoreApplication::applicationDirPath() );
    QStringList filters;
    filters.clear();
    filters << "FTNoIR_Protocol_*.dll";
	protocolFileList.clear();
	listDLLs.clear();
	listDLLs = settingsDir.entryList( filters, QDir::Files, QDir::Name );

	//
	// Add strings to the Listbox.
	//
	disconnect(ui.iconcomboProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolSelected(int)));
	ui.iconcomboProtocol->clear();
	for ( int i = 0; i < listDLLs.size(); i++) {

		// Try to load the DLL and get the Icon and Name
		QLibrary *protocolLib = new QLibrary(listDLLs.at(i));
		QString *protocolName = new QString("");
		QIcon *protocolIcon = new QIcon();

		getProtocol = (importGetProtocolDll) protocolLib->resolve("GetProtocolDll");
		if (getProtocol) {
			IProtocolDllPtr ptrXyz(getProtocol());
			if (ptrXyz)
			{
				pProtocolDll = ptrXyz;
				pProtocolDll->getFullName( protocolName );
				pProtocolDll->getIcon( protocolIcon );

				//
				// Add the Icon and the Name to the Listbox and update the fileList
				//
				ui.iconcomboProtocol->addItem(*protocolIcon, *protocolName );
				protocolFileList.append(listDLLs.at(i));
			}
		}
		else {
			QMessageBox::warning(0,"FaceTrackNoIR Error", "Protocol-DLL not loaded, please check if the DLL is version 1.7 \nand all dependencies are installed. \n(" + listDLLs.at(i) + ")",QMessageBox::Ok,QMessageBox::NoButton);
		}

	}
	connect(ui.iconcomboProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolSelected(int)));

	//
	// Get a List of all the Filter-DLL-files in the Program-folder.
	//
    filters.clear();
    filters << "FTNoIR_Filter_*.dll";
	filterFileList.clear();
	listDLLs.clear();
	listDLLs = settingsDir.entryList( filters, QDir::Files, QDir::Name );

	//
	// Add strings to the Listbox.
	//
	disconnect(ui.iconcomboFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(filterSelected(int)));
	ui.iconcomboFilter->clear();
	ui.iconcomboFilter->addItem("None");

	for ( int i = 0; i < listDLLs.size(); i++) {

		// Try to load the DLL and get the Icon and Name
		QLibrary *filterLib = new QLibrary(listDLLs.at(i));
		QString *filterName = new QString("");
		QIcon *filterIcon = new QIcon();

		getFilter = (importGetFilterDll) filterLib->resolve("GetFilterDll");
		if (getFilter) {
			IFilterDllPtr ptrXyz(getFilter());
			if (ptrXyz)
			{
				pFilterDll = ptrXyz;
				pFilterDll->getFullName( filterName );
				pFilterDll->getIcon( filterIcon );

				//
				// Add the Icon and the Name to the Listbox and update the fileList
				//
				ui.iconcomboFilter->addItem(*filterIcon, *filterName );
				filterFileList.append(listDLLs.at(i));
			}
		}
		else {
			QMessageBox::warning(0,"FaceTrackNoIR Error", "Filter-DLL not loaded, please check if the DLL is version 1.7 \nand all dependencies are installed. \n(" + listDLLs.at(i) + ")",QMessageBox::Ok,QMessageBox::NoButton);
		}

	}
	connect(ui.iconcomboFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(filterSelected(int)));

	//
	// Get a List of all the Tracker-DLL-files in the Program-folder.
	//
    filters.clear();
	filters << "FTNoIR_Tracker_*.dll";
	trackerFileList.clear();
	listDLLs.clear();
	listDLLs = settingsDir.entryList( filters, QDir::Files, QDir::Name );

	//
	// Add strings to the Listbox(es).
	//
	disconnect(ui.iconcomboTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	ui.iconcomboTrackerSource->clear();

	disconnect(ui.cbxSecondTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	ui.cbxSecondTrackerSource->clear();
	ui.cbxSecondTrackerSource->addItem("None");

	for ( int i = 0; i < listDLLs.size(); i++) {

		// Try to load the DLL and get the Icon and Name
		QLibrary *trackerLib = new QLibrary(listDLLs.at(i));
		QString *trackerName = new QString("");
		QIcon *trackerIcon = new QIcon();

		getTracker = (importGetTrackerDll) trackerLib->resolve("GetTrackerDll");
		if (getTracker) {
			ITrackerDll *ptrXyz(getTracker());
			if (ptrXyz)
			{
				pTrackerDll = ptrXyz;
				pTrackerDll->getFullName( trackerName );
				pTrackerDll->getIcon( trackerIcon );

				//
				// Add the Icon and the Name to the Listbox and update the fileList
				//
				ui.iconcomboTrackerSource->addItem(*trackerIcon, *trackerName );
				ui.cbxSecondTrackerSource->addItem(*trackerIcon, *trackerName );
				trackerFileList.append(listDLLs.at(i));
			}
		}
		else {
			QMessageBox::warning(0,"FaceTrackNoIR Error", "Tracker-DLL not loaded, please check if the DLL is version 1.7 \nand all dependencies are installed. \n(" + listDLLs.at(i) + ")",QMessageBox::Ok,QMessageBox::NoButton);
		}

	}
	connect(ui.iconcomboTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
	connect(ui.cbxSecondTrackerSource, SIGNAL(currentIndexChanged(int)), this, SLOT(trackingSourceSelected(int)));
}

//
// Create the Actions in the System tray and connect them to Application events
//
void FaceTrackNoIR::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize FaceTrackNoIR"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    //maximizeAction = new QAction(tr("Ma&ximize"), this);
    //connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    restoreAction = new QAction(tr("&Restore FaceTrackNoIR"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    quitAction = new QAction(tr("&Quit FaceTrackNoIR"), this);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

//
// Create the SystemTray and set the default Icon
//
void FaceTrackNoIR::createTrayIcon()
{
	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		trayIconMenu = new QMenu(this);
		trayIconMenu->addAction(minimizeAction);
		trayIconMenu->addAction(restoreAction);
		trayIconMenu->addSeparator();
		trayIconMenu->addAction(quitAction);

		trayIcon = new QSystemTrayIcon(this);
		trayIcon->setContextMenu(trayIconMenu);

		trayIcon->setIcon(QIcon(QCoreApplication::applicationDirPath() + "/images/FaceTrackNoIR.ico"));
	}
}

//
// Handle SystemTray events
//
void FaceTrackNoIR::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
     switch (reason) {
     case QSystemTrayIcon::Trigger:
     case QSystemTrayIcon::DoubleClick:
         //ui.iconcomboProtocol->setCurrentIndex((ui.iconcomboProtocol->currentIndex() + 1)
         //                              % ui.iconcomboProtocol->count());
         break;
     ////case QSystemTrayIcon::MiddleClick:
     ////    showMessage();
     ////    break;
     default:
         ;
     }
 }

//
// Handle changes of the Protocol selection
//
void FaceTrackNoIR::protocolSelected(int index)
{
	settingsDirty = true;
	ui.btnShowServerControls->setEnabled ( true );

	//
	// Set the Icon for the tray and update the Icon for the Settings button.
	//
	QIcon icon = ui.iconcomboProtocol->itemIcon(index);
	if (trayIcon != 0) {
		trayIcon->setIcon(icon);
	    trayIcon->setToolTip(ui.iconcomboProtocol->itemText(index));
		trayIcon->show();
		trayIcon->showMessage( "FaceTrackNoIR", ui.iconcomboProtocol->itemText(index));
	}
	setWindowIcon(QIcon(":/images/FaceTrackNoIR.ico"));
	ui.btnShowServerControls->setIcon(icon);
}

//
// Handle changes of the Tracking Source selection
//
void FaceTrackNoIR::trackingSourceSelected(int index)
{
	settingsDirty = true;
	ui.btnShowEngineControls->setEnabled ( true );
}

//
// Handle changes of the Profile selection
//
void FaceTrackNoIR::profileSelected(int index)
{
	//
	// Read the current INI-file setting, to get the folder in which it's located...
	//
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
    QFileInfo pathInfo ( currentFile );

	//
	// Save the name of the INI-file in the Registry.
	//
	settings.setValue ("SettingsFile", pathInfo.absolutePath() + "/" + iniFileList.at(ui.iconcomboProfile->currentIndex()));
	loadSettings();
}

//
// Handle changes of the Filter selection
//
void FaceTrackNoIR::filterSelected(int index)
{
	settingsDirty = true;

	//QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	//QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	//QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	ui.btnShowFilterControls->setEnabled ( true );
}

//
// Constructor for FaceTrackNoIR=Preferences-dialog
//
PreferencesDialog::PreferencesDialog( FaceTrackNoIR *ftnoir, QWidget *parent, Qt::WindowFlags f ) :
QWidget( parent , f)
{
	ui.setupUi( this );

	QPoint offsetpos(100, 100);
	this->move(parent->pos() + offsetpos);

	mainApp = ftnoir;											// Preserve a pointer to FTNoIR

	// Connect Qt signals to member-functions
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(doOK()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(doCancel()));

	connect(ui.spinAutoMinimizeTime, SIGNAL(valueChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkAutoStartTracking, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.radioMinimize, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	// Load the settings from the current .INI-file
	loadSettings();
}

//
// Destructor for server-dialog
//
PreferencesDialog::~PreferencesDialog() {
	qDebug() << "~PreferencesDialog() says: started";
}

//
// OK clicked on server-dialog
//
void PreferencesDialog::doOK() {
	save();
	this->close();
}

// override show event
void PreferencesDialog::showEvent ( QShowEvent * event ) {
	loadSettings();
}

//
// Cancel clicked on server-dialog
//
void PreferencesDialog::doCancel() {
	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard );

		qDebug() << "doCancel says: answer =" << ret;

		switch (ret) {
			case QMessageBox::Save:
				save();
				this->close();
				break;
			case QMessageBox::Discard:
				this->close();
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
	else {
		this->close();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void PreferencesDialog::loadSettings() {

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	ui.spinAutoMinimizeTime->setValue( settings.value ( "AutoMinimizeTime", 0 ).toInt() );
	ui.chkAutoStartTracking->setChecked( settings.value ( "AutoStartTracking", 0 ).toBool() );
	ui.radioMinimize->setChecked( settings.value ( "MinimizeTaskBar", 1 ).toBool() );

	settingsDirty = false;

}

//
// Save the current Settings to the currently 'active' INI-file.
//
void PreferencesDialog::save() {

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	settings.setValue( "AutoMinimizeTime", ui.spinAutoMinimizeTime->value() );
	settings.setValue( "AutoStartTracking", ui.chkAutoStartTracking->isChecked() );
	settings.setValue( "MinimizeTaskBar", ui.radioMinimize->isChecked() );

	//
	// Send a message to the main program, to update the Settings (for the tracker)
	//
	mainApp->updateSettings();

	settingsDirty = false;
}

//**************************************************************************************************//
//**************************************************************************************************//
//
// Constructor for Keyboard-shortcuts-dialog
//
KeyboardShortcutDialog::KeyboardShortcutDialog( FaceTrackNoIR *ftnoir, QWidget *parent, Qt::WindowFlags f ) :
QWidget( parent , f)
{
	ui.setupUi( this );

	QPoint offsetpos(100, 100);
	this->move(parent->pos() + offsetpos);

	mainApp = ftnoir;											// Preserve a pointer to FTNoIR

	// Connect Qt signals to member-functions
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(doOK()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(doCancel()));

	connect(ui.cbxCenterKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.cbxCenterMouseKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.chkCenterShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkCenterCtrl, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkCenterAlt, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	connect(ui.cbxGameZeroKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.cbxGameZeroMouseKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.chkGameZeroShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkGameZeroCtrl, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkGameZeroAlt, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	connect(ui.cbxStartStopKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.cbxStartStopMouseKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.chkStartStopShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkStartStopCtrl, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkStartStopAlt, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.radioSetZero, SIGNAL(toggled(bool)), this, SLOT(keyChanged(bool)));
	connect(ui.radioSetEngineStop, SIGNAL(toggled(bool)), this, SLOT(keyChanged(bool)));

	connect(ui.cbxInhibitKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.cbxInhibitMouseKey, SIGNAL(currentIndexChanged(int)), this, SLOT(keyChanged( int )));
	connect(ui.chkInhibitShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitCtrl, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitAlt, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	// Also add events for the Axis-checkboxes
	connect(ui.chkInhibitShift, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitYaw, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitRoll, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitX, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitY, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
	connect(ui.chkInhibitZ, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));

	//
	// Clear the Lists with key-descriptions and keycodes and build the Lists
	// The strings will all be added to the ListBoxes for each Shortkey
	//
	stringList.clear();
	stringList.append("NONE");
	stringList.append("F1");
	stringList.append("F2");
	stringList.append("F3");
	stringList.append("F4");
	stringList.append("F5");
	stringList.append("F6");
	stringList.append("F7");
	stringList.append("F8");
	stringList.append("F9");
	stringList.append("F10");
	stringList.append("F11");
	stringList.append("F12");
	stringList.append("MINUS");
	stringList.append("EQUALS");
	stringList.append("BACK");
	stringList.append("A");
	stringList.append("B");
	stringList.append("C");
	stringList.append("D");
	stringList.append("E");
	stringList.append("F");
	stringList.append("G");
	stringList.append("H");
	stringList.append("I");
	stringList.append("J");
	stringList.append("K");
	stringList.append("L");
	stringList.append("M");
	stringList.append("N");
	stringList.append("O");
	stringList.append("P");
	stringList.append("Q");
	stringList.append("R");
	stringList.append("S");
	stringList.append("T");
	stringList.append("U");
	stringList.append("V");
	stringList.append("W");
	stringList.append("X");
	stringList.append("Y");
	stringList.append("Z");
	stringList.append("NUMPAD0");
	stringList.append("NUMPAD1");
	stringList.append("NUMPAD2");
	stringList.append("NUMPAD3");
	stringList.append("NUMPAD4");
	stringList.append("NUMPAD5");
	stringList.append("NUMPAD6");
	stringList.append("NUMPAD7");
	stringList.append("NUMPAD8");
	stringList.append("NUMPAD9");
	stringList.append("HOME");
	stringList.append("UP");
	stringList.append("PGUP");		/* PgUp on arrow keypad */
	stringList.append("LEFT");
	stringList.append("RIGHT");
	stringList.append("END");
	stringList.append("DOWN");
	stringList.append("PGDWN");		/* PgDn on arrow keypad */
	stringList.append("INSERT");
	stringList.append("DELETE");

	keyList.clear();
	keyList.append(0);				// NONE = 0
	keyList.append(DIK_F1);
	keyList.append(DIK_F2);
	keyList.append(DIK_F3);
	keyList.append(DIK_F4);
	keyList.append(DIK_F5);
	keyList.append(DIK_F6);
	keyList.append(DIK_F7);
	keyList.append(DIK_F8);
	keyList.append(DIK_F9);
	keyList.append(DIK_F10);
	keyList.append(DIK_F11);
	keyList.append(DIK_F12);
	keyList.append(DIK_MINUS);
	keyList.append(DIK_EQUALS);
	keyList.append(DIK_BACK);
	keyList.append(DIK_A);
	keyList.append(DIK_B);
	keyList.append(DIK_C);
	keyList.append(DIK_D);
	keyList.append(DIK_E);
	keyList.append(DIK_F);
	keyList.append(DIK_G);
	keyList.append(DIK_H);
	keyList.append(DIK_I);
	keyList.append(DIK_J);
	keyList.append(DIK_K);
	keyList.append(DIK_L);
	keyList.append(DIK_M);
	keyList.append(DIK_N);
	keyList.append(DIK_O);
	keyList.append(DIK_P);
	keyList.append(DIK_Q);
	keyList.append(DIK_R);
	keyList.append(DIK_S);
	keyList.append(DIK_T);
	keyList.append(DIK_U);
	keyList.append(DIK_V);
	keyList.append(DIK_W);
	keyList.append(DIK_X);
	keyList.append(DIK_Y);
	keyList.append(DIK_Z);
	keyList.append(DIK_NUMPAD0);
	keyList.append(DIK_NUMPAD1);
	keyList.append(DIK_NUMPAD2);
	keyList.append(DIK_NUMPAD3);
	keyList.append(DIK_NUMPAD4);
	keyList.append(DIK_NUMPAD5);
	keyList.append(DIK_NUMPAD6);
	keyList.append(DIK_NUMPAD7);
	keyList.append(DIK_NUMPAD8);
	keyList.append(DIK_NUMPAD9);
	keyList.append(DIK_HOME);
	keyList.append(DIK_UP);
	keyList.append(DIK_PRIOR);		/* PgUp on arrow keypad */
	keyList.append(DIK_LEFT);
	keyList.append(DIK_RIGHT);
	keyList.append(DIK_END);
	keyList.append(DIK_DOWN);
	keyList.append(DIK_NEXT);		/* PgDn on arrow keypad */
	keyList.append(DIK_INSERT);
	keyList.append(DIK_DELETE);

	//
	// Add strings to the Listboxes.
	//
	for ( int i = 0; i < stringList.size(); i++) {
		ui.cbxCenterKey->addItem(stringList.at(i));
		ui.cbxGameZeroKey->addItem(stringList.at(i));
		ui.cbxStartStopKey->addItem(stringList.at(i));
		ui.cbxInhibitKey->addItem(stringList.at(i));
	}

	//
	// Clear the Lists with key-descriptions and keycodes and build the Lists
	// The strings will all be added to the ListBoxes for each Shortkey
	//
	stringListMouse.clear();
	stringListMouse.append("NONE");
	stringListMouse.append("LEFT");
	stringListMouse.append("RIGHT");
	stringListMouse.append("MIDDLE");
	stringListMouse.append("BACK");
	stringListMouse.append("FORWARD");

	//
	// Add strings to the Listboxes.
	//
	for ( int i = 0; i < stringListMouse.size(); i++) {
		ui.cbxCenterMouseKey->addItem(stringListMouse.at(i));
		ui.cbxGameZeroMouseKey->addItem(stringListMouse.at(i));
		ui.cbxStartStopMouseKey->addItem(stringListMouse.at(i));
		ui.cbxInhibitMouseKey->addItem(stringListMouse.at(i));
	}

	// Load the settings from the current .INI-file
	loadSettings();
}

//
// Destructor for server-dialog
//
KeyboardShortcutDialog::~KeyboardShortcutDialog() {
	qDebug() << "~KeyboardShortcutDialog() says: started";
}

//
// OK clicked on server-dialog
//
void KeyboardShortcutDialog::doOK() {
	save();
	this->close();
}

// override show event
void KeyboardShortcutDialog::showEvent ( QShowEvent * event ) {
	loadSettings();
}

//
// Cancel clicked on server-dialog
//
void KeyboardShortcutDialog::doCancel() {
	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard );

		qDebug() << "doCancel says: answer =" << ret;

		switch (ret) {
			case QMessageBox::Save:
				save();
				this->close();
				break;
			case QMessageBox::Discard:
				this->close();
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
	else {
		this->close();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void KeyboardShortcutDialog::loadSettings() {
int keyindex;

	qDebug() << "loadSettings says: Starting ";
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	qDebug() << "loadSettings says: iniFile = " << currentFile;

	iniFile.beginGroup ( "KB_Shortcuts" );
	
	// Center key
	ui.cbxCenterMouseKey->setCurrentIndex( iniFile.value ( "MouseKey_Center", 0 ).toInt() );
	keyindex = keyList.indexOf ( iniFile.value ( "Keycode_Center", DIK_HOME ).toInt() );
	if ( keyindex > 0 ) {
		ui.cbxCenterKey->setCurrentIndex( keyindex );
	}
	else {
		ui.cbxCenterKey->setCurrentIndex( 0 );
	}
	ui.chkCenterShift->setChecked (iniFile.value ( "Shift_Center", 0 ).toBool());
	ui.chkCenterCtrl->setChecked (iniFile.value ( "Ctrl_Center", 0 ).toBool());
	ui.chkCenterAlt->setChecked (iniFile.value ( "Alt_Center", 0 ).toBool());
	ui.chkDisableBeep->setChecked (iniFile.value ( "Disable_Beep", 0 ).toBool());

	// GameZero key
	ui.cbxGameZeroMouseKey->setCurrentIndex( iniFile.value ( "MouseKey_GameZero", 0 ).toInt() );
	keyindex = keyList.indexOf ( iniFile.value ( "Keycode_GameZero", 1 ).toInt() );
	if ( keyindex > 0 ) {
		ui.cbxGameZeroKey->setCurrentIndex( keyindex );
	}
	else {
		ui.cbxGameZeroKey->setCurrentIndex( 0 );
	}
	ui.chkGameZeroShift->setChecked (iniFile.value ( "Shift_GameZero", 0 ).toBool());
	ui.chkGameZeroCtrl->setChecked (iniFile.value ( "Ctrl_GameZero", 0 ).toBool());
	ui.chkGameZeroAlt->setChecked (iniFile.value ( "Alt_GameZero", 0 ).toBool());

	// Start/stop key
	ui.cbxStartStopMouseKey->setCurrentIndex( iniFile.value ( "MouseKey_StartStop", 0 ).toInt() );
	keyindex = keyList.indexOf ( iniFile.value ( "Keycode_StartStop", DIK_END ).toInt() );
	if ( keyindex > 0 ) {
		ui.cbxStartStopKey->setCurrentIndex( keyindex );
	}
	else {
		ui.cbxStartStopKey->setCurrentIndex( 0 );
	}
	ui.chkStartStopShift->setChecked (iniFile.value ( "Shift_StartStop", 0 ).toBool());
	ui.chkStartStopCtrl->setChecked (iniFile.value ( "Ctrl_StartStop", 0 ).toBool());
	ui.chkStartStopAlt->setChecked (iniFile.value ( "Alt_StartStop", 0 ).toBool());
	ui.radioSetZero->setChecked (iniFile.value ( "SetZero", 1 ).toBool());
	ui.radioSetFreeze->setChecked(!ui.radioSetZero->isChecked());
	ui.radioSetEngineStop->setChecked (iniFile.value ( "SetEngineStop", 1 ).toBool());
	ui.radioSetKeepTracking->setChecked(!ui.radioSetEngineStop->isChecked());

	// Axis-inhibitor key
	ui.cbxInhibitMouseKey->setCurrentIndex( iniFile.value ( "MouseKey_Inhibit", 0 ).toInt() );
	keyindex = keyList.indexOf ( iniFile.value ( "Keycode_Inhibit", 1 ).toInt() );
	if ( keyindex > 0 ) {
		ui.cbxInhibitKey->setCurrentIndex( keyindex );
	}
	else {
		ui.cbxInhibitKey->setCurrentIndex( 0 );
	}
	ui.chkInhibitShift->setChecked (iniFile.value ( "Shift_Inhibit", 0 ).toBool());
	ui.chkInhibitCtrl->setChecked (iniFile.value ( "Ctrl_Inhibit", 0 ).toBool());
	ui.chkInhibitAlt->setChecked (iniFile.value ( "Alt_Inhibit", 0 ).toBool());

	ui.chkInhibitPitch->setChecked (iniFile.value ( "Inhibit_Pitch", 0 ).toBool());
	ui.chkInhibitYaw->setChecked (iniFile.value ( "Inhibit_Yaw", 0 ).toBool());
	ui.chkInhibitRoll->setChecked (iniFile.value ( "Inhibit_Roll", 0 ).toBool());
	ui.chkInhibitX->setChecked (iniFile.value ( "Inhibit_X", 0 ).toBool());
	ui.chkInhibitY->setChecked (iniFile.value ( "Inhibit_Y", 0 ).toBool());
	ui.chkInhibitZ->setChecked (iniFile.value ( "Inhibit_Z", 0 ).toBool());


	// Reverse Axis
	ui.chkEnableReverseAxis->setChecked (iniFile.value ( "Enable_ReverseAxis", 0 ).toBool());
	ui.spinYawAngle4ReverseAxis->setValue( iniFile.value ( "RA_Yaw", 40 ).toInt() );
	ui.spinZ_Pos4ReverseAxis->setValue( iniFile.value ( "RA_ZPos", -20 ).toInt() );
	ui.spinZ_PosWhenReverseAxis->setValue( iniFile.value ( "RA_ToZPos", 50 ).toInt() );

	iniFile.endGroup ();

	settingsDirty = false;

}

//
// Save the current Settings to the currently 'active' INI-file.
//
void KeyboardShortcutDialog::save() {

	qDebug() << "save() says: started";

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	iniFile.beginGroup ( "KB_Shortcuts" );
	iniFile.setValue ( "MouseKey_Center", ui.cbxCenterMouseKey->currentIndex());
	iniFile.setValue ( "Keycode_Center", keyList.at( ui.cbxCenterKey->currentIndex() ) );
	iniFile.setValue ( "Shift_Center", ui.chkCenterShift->isChecked() );
	iniFile.setValue ( "Ctrl_Center", ui.chkCenterCtrl->isChecked() );
	iniFile.setValue ( "Alt_Center", ui.chkCenterAlt->isChecked() );
	iniFile.setValue ( "Disable_Beep", ui.chkDisableBeep->isChecked() );

	iniFile.setValue ( "MouseKey_GameZero", ui.cbxGameZeroMouseKey->currentIndex());
	iniFile.setValue ( "Keycode_GameZero", keyList.at( ui.cbxGameZeroKey->currentIndex() ) );
	iniFile.setValue ( "Shift_GameZero", ui.chkGameZeroShift->isChecked() );
	iniFile.setValue ( "Ctrl_GameZero", ui.chkGameZeroCtrl->isChecked() );
	iniFile.setValue ( "Alt_GameZero", ui.chkGameZeroAlt->isChecked() );

	iniFile.setValue ( "MouseKey_StartStop", ui.cbxStartStopMouseKey->currentIndex());
	iniFile.setValue ( "Keycode_StartStop", keyList.at( ui.cbxStartStopKey->currentIndex() ) );
	iniFile.setValue ( "Shift_StartStop", ui.chkStartStopShift->isChecked() );
	iniFile.setValue ( "Ctrl_StartStop", ui.chkStartStopCtrl->isChecked() );
	iniFile.setValue ( "Alt_StartStop", ui.chkStartStopAlt->isChecked() );
	iniFile.setValue ( "SetZero", ui.radioSetZero->isChecked() );
	iniFile.setValue ( "SetEngineStop", ui.radioSetEngineStop->isChecked() );

	iniFile.setValue ( "MouseKey_Inhibit", ui.cbxInhibitMouseKey->currentIndex());
	iniFile.setValue ( "Keycode_Inhibit", keyList.at( ui.cbxInhibitKey->currentIndex() ) );
	iniFile.setValue ( "Shift_Inhibit", ui.chkInhibitShift->isChecked() );
	iniFile.setValue ( "Ctrl_Inhibit", ui.chkInhibitCtrl->isChecked() );
	iniFile.setValue ( "Alt_Inhibit", ui.chkInhibitAlt->isChecked() );

	iniFile.setValue ( "Inhibit_Pitch", ui.chkInhibitPitch->isChecked() );
	iniFile.setValue ( "Inhibit_Yaw", ui.chkInhibitYaw->isChecked() );
	iniFile.setValue ( "Inhibit_Roll", ui.chkInhibitRoll->isChecked() );
	iniFile.setValue ( "Inhibit_X", ui.chkInhibitX->isChecked() );
	iniFile.setValue ( "Inhibit_Y", ui.chkInhibitY->isChecked() );
	iniFile.setValue ( "Inhibit_Z", ui.chkInhibitZ->isChecked() );

	// Reverse Axis
	iniFile.setValue ( "Enable_ReverseAxis", ui.chkEnableReverseAxis->isChecked() );
	iniFile.setValue( "RA_Yaw", ui.spinYawAngle4ReverseAxis->value() );
	iniFile.setValue( "RA_ZPos", ui.spinZ_Pos4ReverseAxis->value() );
	iniFile.setValue( "RA_ToZPos", ui.spinZ_PosWhenReverseAxis->value() );

	iniFile.endGroup ();

	settingsDirty = false;

	//
	// Send a message to the main program, to update the Settings (for the tracker)
	//
	mainApp->updateSettings();
}

//**************************************************************************************************//
//**************************************************************************************************//
//
// Constructor for Curve-configuration-dialog
//
CurveConfigurationDialog::CurveConfigurationDialog( FaceTrackNoIR *ftnoir, QWidget *parent, Qt::WindowFlags f ) :
QWidget( parent , f)
{
	ui.setupUi( this );

	QPoint offsetpos(120, 30);
	this->move(parent->pos() + offsetpos);

	mainApp = ftnoir;											// Preserve a pointer to FTNoIR

	// Connect Qt signals to member-functions
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(doOK()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(doCancel()));

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)
	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();

	ui.qFunctionX->setConfig(Tracker::X.curvePtr, currentFile);
	connect(ui.qFunctionX, SIGNAL(CurveChanged(bool)), this, SLOT(curveChanged(bool)));
	ui.qFunctionY->setConfig(Tracker::Y.curvePtr, currentFile);
	connect(ui.qFunctionY, SIGNAL(CurveChanged(bool)), this, SLOT(curveChanged(bool)));
	ui.qFunctionZ->setConfig(Tracker::Z.curvePtr, currentFile);
	connect(ui.qFunctionZ, SIGNAL(CurveChanged(bool)), this, SLOT(curveChanged(bool)));

	ui.qFunctionYaw->setConfig(Tracker::Yaw.curvePtr, currentFile);
	connect(ui.qFunctionYaw, SIGNAL(CurveChanged(bool)), this, SLOT(curveChanged(bool)));
	//
	// There are 2 curves for Pitch: Up and Down. Users have indicated that, to be able to use visual Flight controls, it is necessary to have a 'slow' curve for Down...
	//
	ui.qFunctionPitch->setConfig(Tracker::Pitch.curvePtr, currentFile);
	connect(ui.qFunctionPitch, SIGNAL(CurveChanged(bool)), this, SLOT(curveChanged(bool)));
	ui.qFunctionPitchDown->setConfig(Tracker::Pitch.curvePtrAlt, currentFile);							
	connect(ui.qFunctionPitchDown, SIGNAL(CurveChanged(bool)), this, SLOT(curveChanged(bool)));

	ui.qFunctionRoll->setConfig(Tracker::Roll.curvePtr, currentFile);
	connect(ui.qFunctionRoll, SIGNAL(CurveChanged(bool)), this, SLOT(curveChanged(bool)));

	// Load the settings from the current .INI-file
	loadSettings();
}

//
// Destructor for server-dialog
//
CurveConfigurationDialog::~CurveConfigurationDialog() {
	qDebug() << "~CurveConfigurationDialog() says: started";
}

//
// OK clicked on server-dialog
//
void CurveConfigurationDialog::doOK() {
	save();
	this->close();
}

// override show event
void CurveConfigurationDialog::showEvent ( QShowEvent * event ) {
	loadSettings();
}

//
// Cancel clicked on server-dialog
//
void CurveConfigurationDialog::doCancel() {
	//
	// Ask if changed Settings should be saved
	//
	if (settingsDirty) {
		int ret = QMessageBox::question ( this, "Settings have changed", "Do you want to save the settings?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard );

		qDebug() << "doCancel says: answer =" << ret;

		switch (ret) {
			case QMessageBox::Save:
				save();
				this->close();
				break;
			case QMessageBox::Discard:
				this->close();
				break;
			case QMessageBox::Cancel:
				// Cancel was clicked
				break;
			default:
				// should never be reached
			break;
		}
	}
	else {
		this->close();
	}
}

//
// Load the current Settings from the currently 'active' INI-file.
//
void CurveConfigurationDialog::loadSettings() {
int NeutralZone;
int sensYaw, sensPitch, sensRoll;
int sensX, sensY, sensZ;

	qDebug() << "loadSettings says: Starting ";
	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	qDebug() << "loadSettings says: iniFile = " << currentFile;

	iniFile.beginGroup ( "Tracking" );
	NeutralZone = iniFile.value ( "NeutralZone", 5 ).toInt();
	sensYaw = iniFile.value ( "sensYaw", 100 ).toInt();
	sensPitch = iniFile.value ( "sensPitch", 100 ).toInt();
	sensRoll = iniFile.value ( "sensRoll", 100 ).toInt();
	sensX = iniFile.value ( "sensX", 100 ).toInt();
	sensY = iniFile.value ( "sensY", 100 ).toInt();
	sensZ = iniFile.value ( "sensZ", 100 ).toInt();

	iniFile.endGroup ();

	ui.qFunctionYaw->loadSettings(currentFile);
	ui.qFunctionPitch->loadSettings(currentFile);
	ui.qFunctionPitchDown->loadSettings(currentFile);
	ui.qFunctionRoll->loadSettings(currentFile);

	settingsDirty = false;

}

//
// Save the current Settings to the currently 'active' INI-file.
//
void CurveConfigurationDialog::save() {

	qDebug() << "save() says: started";

	QSettings settings("Abbequerque Inc.", "FaceTrackNoIR");	// Registry settings (in HK_USER)

	QString currentFile = settings.value ( "SettingsFile", QCoreApplication::applicationDirPath() + "/Settings/default.ini" ).toString();
	QSettings iniFile( currentFile, QSettings::IniFormat );		// Application settings (in INI-file)

	ui.qFunctionYaw->saveSettings(currentFile);
	ui.qFunctionPitch->saveSettings(currentFile);
	ui.qFunctionPitchDown->saveSettings(currentFile);
	ui.qFunctionRoll->saveSettings(currentFile);

	ui.qFunctionX->saveSettings(currentFile);
    ui.qFunctionY->saveSettings(currentFile);
	ui.qFunctionZ->saveSettings(currentFile);

	settingsDirty = false;

	//
	// Send a message to the main program, to update the Settings (for the tracker)
	//
	mainApp->updateSettings();
}
