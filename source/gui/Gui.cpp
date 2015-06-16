#include "source/gui/Gui.h"

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QStatusBar>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

#include "source/core/settings/Settings.h"
#include "source/util/stdext.h"
#include "source/core/TrackingThread.h"
#include "source/core/Registry.h"
#include "source/core/serialization/SerializationData.h"


using GUIPARAM::MediaType;

Gui::Gui(Settings &settings, QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , _settings(settings)
    , _trackingThread(std::make_unique<TrackingThread>(_settings))
    , _videoMode(GUIPARAM::VideoMode::Init)
    , _mediaType(_settings.getValueOrDefault<MediaType>(GUIPARAM::MEDIA_TYPE, MediaType::NoMedia))
    , _isPanZoomMode(false)
    , _currentFrame(0)
{
	ui.setupUi(this);

	initGui();
	initConnects();
}

Gui::~Gui()
{
	_trackingThread->stop();
}

void Gui::startUp()
{
	if (_mediaType != MediaType::NoMedia) initPlayback();

	{
		QFile file(QString::fromStdString(CONFIGPARAM::GEOMETRY_FILE));
		if (file.open(QIODevice::ReadOnly)) restoreGeometry(file.readAll());
	}
	{
		QFile file(QString::fromStdString(CONFIGPARAM::STATE_FILE));
		if (file.open(QIODevice::ReadOnly)) restoreState(file.readAll());
	}
	{
		const auto tracker = _settings.maybeGetValueOfParam<std::string>(TRACKERPARAM::SELECTED_TRACKER);
		if (tracker) {
			const int index = ui.cb_algorithms->findText(QString::fromStdString(tracker.get()));
			if (index != -1) {
				ui.cb_algorithms->setCurrentIndex(index);
			}
		}
	}
}

//function to test file existence
inline bool file_exist(const std::string& name) {
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	} else {
		return false;
	}
}

void Gui::initGui()
{
	_iconPause.addFile(QStringLiteral(":/BioTracker/resources/pause-sign.png"), QSize(), QIcon::Normal, QIcon::Off);
	_iconPlay.addFile(QStringLiteral(":/BioTracker/resources/arrow-forward1.png"), QSize(), QIcon::Normal, QIcon::Off);

	_vboxParams = new QVBoxLayout(ui.groupBox_params);
	_vboxTools  = new QVBoxLayout(ui.groupBox_tools);

	initAlgorithmList();
}

void Gui::showEvent(QShowEvent *ev)
{
	QMainWindow::showEvent(ev);
	emit window_loaded();
}

void Gui::initConnects()
{
	//Start Up
	QObject::connect(this, SIGNAL(window_loaded()), this, SLOT(startUp()), Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

	//File -> Open Video
	QObject::connect(ui.actionOpen_Video, SIGNAL(triggered()), this, SLOT(browseVideo()));
	QObject::connect(ui.actionOpen_Picture, SIGNAL(triggered()), this, SLOT(browsePicture()));	
	//File -> Load/Store tracking data
	QObject::connect(ui.actionLoad_tracking_data, &QAction::triggered,
	                 this, &Gui::loadTrackingDataTriggered);
	QObject::connect(ui.actionSave_tracking_data, &QAction::triggered,
	                 this, &Gui::storeTrackingDataTriggered);
	//File -> Exit
	QObject::connect(ui.actionQuit, &QAction::triggered, this, &Gui::exit);
	//video playfield buttons
	QObject::connect(ui.button_playPause, SIGNAL(clicked()), this, SLOT(runCapture()));
	QObject::connect(ui.button_stop, SIGNAL(clicked()), this, SLOT(stopCapture()));
	QObject::connect(ui.button_nextFrame, SIGNAL( clicked() ), this, SLOT(stepCaptureForward())); 
	QObject::connect(ui.button_previousFrame, SIGNAL( clicked() ), this, SLOT(stepCaptureBackward()));
	QObject::connect(ui.frame_num_edit, SIGNAL( returnPressed() ), this, SLOT( changeCurrentFramebyEdit()));
	QObject::connect(ui.button_screenshot, SIGNAL( clicked() ), this, SLOT( takeScreenshot()));
	QObject::connect(ui.cb_algorithms, SIGNAL( currentIndexChanged ( QString) ), this, SLOT(trackingAlgChanged(QString)));
	QObject::connect(ui.button_panZoom, SIGNAL(  clicked() ), this, SLOT(switchPanZoomMode()));
	QObject::connect(ui.comboBoxSelectedView, SIGNAL(currentIndexChanged(int)), this, SLOT(viewIndexChanged(int)));

	//slider
	QObject::connect(ui.sld_video, SIGNAL(sliderPressed()),this, SLOT(pauseCapture()));
	QObject::connect(ui.sld_video, SIGNAL(sliderMoved(int) ), this, SLOT(updateFrameNumber(int)));
	QObject::connect(ui.sld_video, SIGNAL(sliderReleased() ), this, SLOT(changeCurrentFramebySlider()));
	QObject::connect(ui.sld_video, SIGNAL(actionTriggered(int) ), this, SLOT(changeCurrentFramebySlider(int)));
	QObject::connect(ui.sld_speed, SIGNAL(valueChanged(int) ), this, SLOT(changeFps(int)));
	QObject::connect(ui.videoView, SIGNAL(notifyGUI(std::string, MSGS::MTYPE)), this, SLOT(printGuiMessage(std::string, MSGS::MTYPE)));

	//tracking thread signals
	QObject::connect(_trackingThread.get(), &TrackingThread::notifyGUI, this, &Gui::printGuiMessage);
	QObject::connect(this, &Gui::videoPause, _trackingThread.get(), &TrackingThread::enableVideoPause);
	QObject::connect(this, &Gui::videoStop, _trackingThread.get(), &TrackingThread::stopCapture);
	QObject::connect(_trackingThread.get(), &TrackingThread::trackingSequenceDone, this, &Gui::drawImage);
	QObject::connect(_trackingThread.get(), &TrackingThread::newFrameNumber, this, &Gui::updateFrameNumber);
	QObject::connect(_trackingThread.get(), &TrackingThread::sendFps, this, &Gui::showFps);
	QObject::connect(this, &Gui::nextFrameReady, _trackingThread.get(), &TrackingThread::enableHandlingNextFrame);
	QObject::connect(this, &Gui::changeFrame, _trackingThread.get(), &TrackingThread::setFrameNumber);
	QObject::connect(this, &Gui::grabNextFrame, _trackingThread.get(), &TrackingThread::nextFrame);
	QObject::connect(this, &Gui::fpsChange, _trackingThread.get(), &TrackingThread::setFps);
	QObject::connect(this, &Gui::enableMaxSpeed, _trackingThread.get(), &TrackingThread::setMaxSpeed);
	QObject::connect(this, &Gui::changeTrackingAlg, _trackingThread.get(), &TrackingThread::setTrackingAlgorithm);
	QObject::connect(this, &Gui::changeTrackingAlg, ui.videoView, &VideoView::setTrackingAlgorithm);
	QObject::connect(_trackingThread.get(), &TrackingThread::invalidFile, this, &Gui::invalidFile);
	QObject::connect(_trackingThread.get(), &TrackingThread::fileNameChange, this, &Gui::displayFileName);
	QObject::connect(this, &Gui::changeSelectedView, ui.videoView, &VideoView::changeSelectedView);

    /*   _______________________
    *   |                       |
    *   | connect shortcut keys |
    *   |_______________________| */
	// Pan&Zoom
	QShortcut *shortcutPan = new QShortcut(QKeySequence
		(QString::fromStdString(_settings.getValueOrDefault<std::string>(GUIPARAM::SHORTCUT_ZOOM,"Z"))), this);
	QObject::connect(shortcutPan, SIGNAL(activated()), ui.button_panZoom, SLOT(click()));
	// Play*Pause
	QShortcut *shortcutPlay = new QShortcut(QKeySequence
		(QString::fromStdString(_settings.getValueOrDefault<std::string>(GUIPARAM::SHORTCUT_PLAY,"Space"))), this);
	QObject::connect(shortcutPlay, SIGNAL(activated()), ui.button_playPause, SLOT(click()));
	// Next Frame
	QShortcut *shortcutNext = new QShortcut(QKeySequence
		(QString::fromStdString(_settings.getValueOrDefault<std::string>(GUIPARAM::SHORTCUT_NEXT,"Right"))), this);
	QObject::connect(shortcutNext, SIGNAL(activated()), ui.button_nextFrame, SLOT(click()));
	// Previous Frame
	QShortcut *shortcutPrev = new QShortcut(QKeySequence
		(QString::fromStdString(_settings.getValueOrDefault<std::string>(GUIPARAM::SHORTCUT_PREV,"Left"))), this);
	QObject::connect(shortcutPrev, SIGNAL(activated()), ui.button_previousFrame, SLOT(click()));
}

void Gui::initPlayback()
{
    _currentFrame = _settings.getValueOrDefault<size_t>(GUIPARAM::PAUSED_AT_FRAME, 0);
    updateFrameNumber(static_cast<int>(_currentFrame));
	_trackingThread->enableVideoPause(true);

	switch (_mediaType) {
    case MediaType::Images:
		_trackingThread->loadPictures(_settings.getValueOfParam<std::vector<std::string>>(PICTUREPARAM::PICTURE_FILES));
        break;
    case MediaType::Video:
		_trackingThread->loadVideo(_settings.getValueOfParam<std::string>(CAPTUREPARAM::CAP_VIDEO_FILE));
        break;
    default:
		assert(false);
        break;
	}

	setPlayfieldEnabled(true);
	ui.sld_video->setMaximum(_trackingThread->getVideoLength()-1);
	ui.sld_video->setDisabled(false);
	ui.sld_video->setPageStep(static_cast<int>(_trackingThread->getVideoLength()/20));

	ui.frame_num_edit->setValidator(new QIntValidator(0, _trackingThread->getVideoLength()-1, this));
	ui.frame_num_edit->setEnabled(true);
	ui.sld_speed->setValue(static_cast<int>(_trackingThread->getFps()));

	updateFrameNumber(static_cast<int> (_currentFrame));

	std::stringstream ss;
	double fps = _trackingThread->getFps();
	ss << std::setprecision(5) << fps;
	ui.fps_label->setText(QString::fromStdString(ss.str()));

	setPlayfieldPaused(true);

	_trackingThread->setFrameNumber(static_cast<int>(_currentFrame));

	ui.videoView->fitToWindow();

    setVideoMode(GUIPARAM::VideoMode::Stopped);
}

void Gui::initAlgorithmList()
{
	// add NoTracker first
	ui.cb_algorithms->addItem(QString::fromStdString(
	    Algorithms::Registry::getInstance().stringByType().at(Algorithms::NoTracking)));

	// add Trackers
	for (auto& algByStr : Algorithms::Registry::getInstance().typeByString())
	{
		if (algByStr.second != Algorithms::NoTracking)
		{
			ui.cb_algorithms->addItem(QString::fromStdString(algByStr.first));
		}
	}
}

void Gui::browseVideo()
{
	stopCapture();
	QString filename = QFileDialog::getOpenFileName(this, tr("Open video"), "", tr("video Files (*.avi *.wmv *.mp4)"));
	if(!filename.isEmpty()) {
        _settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());
		_mediaType = MediaType::Video;
		_settings.setParam(CAPTUREPARAM::CAP_VIDEO_FILE, filename.toStdString());
		_settings.setParam(GUIPARAM::MEDIA_TYPE, MediaType::Video);
		initPlayback();
	}
}

void Gui::browsePicture()
{
	stopCapture();
	QStringList  filenames = QFileDialog::getOpenFileNames(this, tr("Open video"), "",
		tr("image Files (*.png *.jpg *.jpeg *.gif *.bmp *.jpe *.ppm *.tiff *.tif *.sr *.ras *.pbm *.pgm *.jp2 *.dib)"));
	if(!filenames.isEmpty()){
		std::vector<std::string> filenamevec;
		for (QString& file : filenames) {
			filenamevec.push_back(file.toStdString());
		}
		_mediaType = MediaType::Images;
		_settings.setParam<std::string>(PICTUREPARAM::PICTURE_FILES, std::move(filenamevec));
		_settings.setParam(GUIPARAM::MEDIA_TYPE, MediaType::Images);
		initPlayback();
	}
}

void Gui::loadTrackingDataTriggered(bool /* checked */)
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Load tracking data"), "", tr("Data Files (*.tdat)"));
	if (!filename.isEmpty()) {
		loadTrackingData(filename.toStdString());
	}
}

void Gui::loadTrackingData(const std::string &filename)
{
	printGuiMessage("Restoring tracking data from " + filename, MSGS::NOTIFICATION);
	std::ifstream is(filename);
	cereal::JSONInputArchive ar(is);

	Serialization::Data sdata;
	ar(sdata);

	if (_tracker) {
		assert(_tracker->getType());
		const std::string trackerType =
				Algorithms::Registry::getInstance().stringByType().at(_tracker->getType().get());

		if (sdata.getType() != trackerType) {
			QMessageBox::warning(this, "Unable to load tracking data",
								 "Tracker type does not match.");
			return;
		}
	} else {
		// try to automatically select the required tracking algorithm
		const auto& typeByString = Algorithms::Registry::getInstance().typeByString();
		const auto it = typeByString.find(sdata.getType());
		if (it != typeByString.end()) {
			const int index = ui.cb_algorithms->findText(QString::fromStdString(sdata.getType()));
			assert (index != -1);
			ui.cb_algorithms->setCurrentIndex(index);
		} else {
			QMessageBox::warning(this, "Unable to load tracking data",
								 "Unknown tracker type.");
			return;
		}
	}

	const boost::optional<std::vector<std::string>> currentFiles = getOpenFiles();

	if (!currentFiles) {
		QMessageBox::warning(this, "Unable to load tracking data",
		                     "No file opened.");
		return;
	}

	const boost::optional<std::string> hash = getFileHash(currentFiles.get().front(), currentFiles.get().size());

	if (!hash) {
		QMessageBox::warning(this, "Unable to load tracking data",
							 "Could not calculate file hash.");
		return;
	}

	if (sdata.getFileSha1Hash() != hash.get()) {
		auto buttonClicked = QMessageBox::warning(this, "Unable to load tracking data",
												  "File hash does not match", QMessageBox::Ok | QMessageBox::Ignore);
		if (buttonClicked == QMessageBox::Ok)
			return;
	}

	_tracker->loadObjects(sdata.getTrackedObjects());
}

void Gui::storeTrackingDataTriggered(bool /* checked */)
{
	if (!_tracker) {
		QMessageBox::warning(this, "Unable to store tracking data",
		                     "No tracker selected.");
		return;
	}

	QFileDialog dialog(this, tr("Save tracking data"));
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDefaultSuffix("tdat");
	dialog.setNameFilter(tr("Data Files (*.tdat)"));
	// set displayed file as default filename:
	if (_filename.lastIndexOf(".") > 0)
		dialog.selectFile(_filename.left(_filename.lastIndexOf(".")));
	if (dialog.exec()) {
		const QStringList filenames = dialog.selectedFiles();
		if (!filenames.empty()) {
			storeTrackingData(filenames.first().toStdString());
		}
	}
}

void Gui::storeTrackingData(const std::string &filename)
{
	assert(_tracker->getType());
	printGuiMessage("Storing tracking data in " + filename, MSGS::NOTIFICATION);

	const std::string trackerType =
	        Algorithms::Registry::getInstance().stringByType().at(_tracker->getType().get());
	const boost::optional<std::vector<std::string>> currentFiles = getOpenFiles();

	if (!currentFiles) {
		QMessageBox::warning(this, "Unable to store tracking data",
		                     "No file opened.");
		return;
	}

	const boost::optional<std::string> hash = getFileHash(currentFiles.get().front(), currentFiles.get().size());

	if (!hash) {
		QMessageBox::warning(this, "Unable to store tracking data",
		                     "Could not calculate file hash.");
		return;
	}

	Serialization::Data sdata(trackerType, hash.get(), getFilenamesFromPaths(currentFiles.get()), _tracker->getObjects());

	std::ofstream ostream(filename, std::ios::binary);
	cereal::JSONOutputArchive archive(ostream);
	archive(std::move(sdata));
}

boost::optional<std::vector<std::string>> Gui::getOpenFiles() const
{
	boost::optional<std::vector<std::string>> filename;
	switch (_mediaType) {
	case MediaType::Images:
		filename = _settings.getValueOfParam<std::vector<std::string>>(PICTUREPARAM::PICTURE_FILES);
		break;
	case MediaType::Video:
		filename = std::vector<std::string>();
		filename.get().push_back(_settings.getValueOfParam<std::string>(CAPTUREPARAM::CAP_VIDEO_FILE));
		break;
	default:
		return boost::optional<std::vector<std::string>>();
	}

	// cap_video_file and picture_file can be set, but empty. therefore, we
	// need to check if the parameter actually contains a file name.
	if (filename && !filename.get().empty()) return filename;

	return boost::optional<std::vector<std::string>>();
}

void Gui::exit()
{
	QApplication::exit();
}

void Gui::setPlayfieldPaused(bool enabled){
	if(enabled) {
		//video is paused
		ui.button_nextFrame->setEnabled(true);
		ui.button_previousFrame->setEnabled(true);
		ui.button_playPause->setIcon(_iconPlay);
	} else {
		//video is playing
		ui.button_nextFrame->setEnabled(false);
		ui.button_previousFrame->setEnabled(false);
		ui.button_playPause->setIcon(_iconPause);
	}
}

void Gui::runCapture()
{	
	switch (_videoMode) {
    case GUIPARAM::VideoMode::Stopped:
		initPlayback();
        setVideoMode ( GUIPARAM::VideoMode::Playing );
		setPlayfieldPaused(false);
		emit videoPause(false);
		break;
    case GUIPARAM::VideoMode::Paused:
        setVideoMode ( GUIPARAM::VideoMode::Playing );
		setPlayfieldPaused(false);
		emit videoPause(false);
		break;
    case GUIPARAM::VideoMode::Playing:
		pauseCapture();
		break;
	default:
		break;
	}

}

void Gui::invalidFile()
{
	setPlayfieldEnabled(false);
    setVideoMode ( GUIPARAM::VideoMode::Stopped );
}

void Gui::setPlayfieldEnabled(bool enabled)
{
	ui.button_playPause->setEnabled(enabled);
	ui.button_nextFrame->setEnabled(enabled);
	ui.button_screenshot->setEnabled(enabled);
	ui.button_previousFrame->setEnabled(enabled);
	ui.button_stop->setEnabled(enabled);
}

void Gui::closeEvent(QCloseEvent* event)
{
	const auto dialog = QMessageBox::warning(this, "Exit",
		"Do you really want to close the application?",
		QMessageBox::Yes | QMessageBox::No);
	if( dialog == QMessageBox::Yes) {
		{
			QFile file(QString::fromStdString(CONFIGPARAM::GEOMETRY_FILE));
			if (file.open(QIODevice::WriteOnly)) file.write(saveGeometry());
		}
		{
			QFile file(QString::fromStdString(CONFIGPARAM::STATE_FILE));
			if (file.open(QIODevice::WriteOnly)) file.write(saveState());
		}
		QMainWindow::closeEvent(event);
	} else {
	  event->ignore();
	}
}

boost::optional<Gui::filehash> Gui::getFileHash(const std::string &filename, const size_t numFiles) const
{
	QCryptographicHash sha1Generator(QCryptographicHash::Sha1);
	QFile file(QString::fromStdString(filename));
	if (file.open(QIODevice::ReadOnly)) {
		// calculate hash from first 4096 bytes of file
		sha1Generator.addData(file.peek(4096));
		sha1Generator.addData(QByteArray::number(static_cast<qulonglong>(numFiles)));
		return QString(sha1Generator.result().toHex()).toStdString();
	}

	return boost::optional<filehash>();
}

std::vector<std::string> Gui::getFilenamesFromPaths(const std::vector<std::string> &paths) const
{
	std::vector<std::string> filenames;
	filenames.reserve(paths.size());
	for (std::string const& path : paths) {
		const QFileInfo fi(QString::fromStdString(path));
		filenames.push_back(fi.baseName().toStdString());
	}
	return filenames;
}


bool Gui::event(QEvent *event)
{
	if (!_tracker) return QMainWindow::event(event);

	const QEvent::Type etype = event->type();
	if (etype == QEvent::MouseButtonPress ||
	    etype == QEvent::MouseButtonRelease ||
	    etype == QEvent::MouseMove ||
	    etype == QEvent::Wheel)
	{
		if (ui.videoView->rect().contains(ui.videoView->mapFromGlobal(QCursor::pos()))) {
			QCoreApplication::sendEvent(_tracker.get(), event);
			return true;
		}
	} else if (etype == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		const std::set<Qt::Key>& keys = _tracker->grabbedKeys();
		if (keys.count(static_cast<Qt::Key>(keyEvent->key()))) {
			QCoreApplication::sendEvent(_tracker.get(), event);
			return true;
		}
	}
	return QMainWindow::event(event);
}

void Gui::stepCaptureForward()
{
    setVideoMode ( GUIPARAM::VideoMode::Paused) ;
	emit grabNextFrame();
	_settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());
}

void Gui::stepCaptureBackward()
{
	if(_currentFrame > 0)
	{
        setVideoMode ( GUIPARAM::VideoMode::Paused) ;
		updateFrameNumber(static_cast<int>(_currentFrame-1));
		emit changeFrame(static_cast<int>(_currentFrame));		
		_settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());
	}
}

void Gui::pauseCapture()
{
    setVideoMode ( GUIPARAM::VideoMode::Paused );
	_trackingThread->enableVideoPause(true);
	setPlayfieldPaused(true);
	_settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());
}

void Gui::stopCapture()
{	
    _trackingThread->stopCapture();

    if (_mediaType != MediaType::NoMedia) {
        updateFrameNumber(0);
        _trackingThread->setFrameNumber(0);
    }
    setVideoMode( GUIPARAM::VideoMode::Stopped );

	_settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());
	setPlayfieldPaused(true);
	ui.sld_video->setDisabled(true);
	ui.cb_algorithms->setCurrentIndex(0);
	trackingAlgChanged(Algorithms::NoTracking);
}

void Gui::updateFrameNumber(int frameNumber)
{
    if (_videoMode != GUIPARAM::VideoMode::Stopped) {
		_currentFrame = frameNumber;

		ui.sld_video->setValue(static_cast<int>(_currentFrame));
		ui.frame_num_edit->setText(QString::number(_currentFrame));
		// check if we reached the end of video
		if (frameNumber == ui.sld_video->maximum()) {
			pauseCapture();
		}
        else if (_videoMode == GUIPARAM::VideoMode::Paused
                 || _videoMode == GUIPARAM::VideoMode::Stopped)
        {
			_settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());
		}
	}
}

void Gui::drawImage(cv::Mat image)
{
	if(image.data)
	{	
		ui.videoView->showImage(image);
	}

	// signals when the frame was drawn, and the TrackerThread can continue to work;
	emit nextFrameReady(true);
}

void Gui::printGuiMessage(std::string message, MSGS::MTYPE mType)
{
	QString msgLine =  "<span style=\"color:blue\">";
	msgLine += QDateTime::currentDateTime().toString("hh:mm:ss");
	msgLine += "</span>&nbsp;&nbsp;&nbsp;";
	switch (mType)
	{
	case MSGS::MTYPE::WARNING:
		msgLine += "<span style=\"color:red\"><b> Warning: </b></span>";
		break;
	case MSGS::MTYPE::FAIL:
		msgLine += "<span style=\"color:red\"><b> Error: </b></span>";
		break;
	default:
		msgLine += " ";
		break;
	}
	msgLine.append(QString::fromStdString(message));
	ui.edit_notification->append(msgLine);
}

void Gui::changeCurrentFramebySlider()
{	
	const int value = ui.sld_video->value();
	emit changeFrame(value);
	updateFrameNumber(value);
	_settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());

}
void Gui::changeCurrentFramebySlider(int SliderAction)
{
	int fNum = static_cast<int>(_currentFrame);
	switch(SliderAction)
	{
	case QSlider::SliderPageStepAdd:
		fNum += ui.sld_video->pageStep();
		if (fNum > ui.sld_video->maximum())
			fNum = ui.sld_video->maximum();
		changeCurrentFramebySlider();
		break;
	case QSlider::SliderPageStepSub:
		fNum -= ui.sld_video->pageStep();
		if(fNum < 0 )
			fNum = 0;
		changeCurrentFramebySlider();
		break;
	default:
		break;
	}
	updateFrameNumber(fNum);	
}
void Gui::changeCurrentFramebyEdit()
{	
	const QString valueStr = ui.frame_num_edit->text();
	//check if string is a number by using regular expression
	QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
	if(re.exactMatch(valueStr))
	{
		int value = valueStr.toInt();
		if(_trackingThread->isReadyForNextFrame())
			emit changeFrame(value);
		_settings.setParam(GUIPARAM::PAUSED_AT_FRAME, QString::number(_currentFrame).toStdString());
		updateFrameNumber(value);
	}	
}
void Gui::changeCurrentFramebySignal(int frameNumber)
{
    if(_trackingThread->isReadyForNextFrame())
        emit changeFrame(frameNumber);
    updateFrameNumber(frameNumber);
}

void Gui::showFps(double fps)
{
	std::stringstream ss;
	ss << std::setprecision(5) << fps;
	//show actual fps
	ui.fps_edit->setText(QString::fromStdString(ss.str()));
}

void Gui::changeFps(int fps)
{
	//maximum slider position will enable maxSpeed
	if(fps > 60)
	{
		emit enableMaxSpeed(true);
		ui.fps_label->setText("max");
	}
	else
	{
		//show target fps
		ui.fps_label->setText(QString::number(fps));
		emit enableMaxSpeed(false);
		emit fpsChange(static_cast<double>(fps));
	}
}

void Gui::trackingAlgChanged(QString trackingAlgStr)
{
	trackingAlgChanged(Algorithms::Registry::getInstance().typeByString().at(trackingAlgStr.toStdString()));
}

void Gui::trackingAlgChanged(Algorithms::Type trackingAlg)
{
	// remove views from previous tracker
	resetViews();

	// calculate file hash of currently opened file
	const boost::optional<std::vector<std::string>> openFiles = getOpenFiles();
	boost::optional<filehash> filehash;
	if (openFiles) filehash = getFileHash(openFiles.get().front(), openFiles.get().size());

	if(_tracker)
	{
		// remove ui containers of old algorithm
		_vboxParams->removeWidget(_paramsWidget.get());
		_vboxTools->removeWidget(_toolsWidget.get());
		_paramsWidget.reset();
		_toolsWidget.reset();

		if (filehash && (_tracker->getType().get() != Algorithms::NoTracking)) {
			// map_type_hashtemp_t maps tracker type to a map that maps
			// a file hash to a QTemporaryFile. We try to find a QTemporaryFile
			// that has been previously created that contains the serialization
			// data of the current tracking algorithm and the currently opened
			// file. If such a QTemporaryFile does not exist yet, we create it
			// now.
			map_hash_temp_t& hashTempMap = _serializationTmpFileMap[_tracker->getType().get()];
			QTemporaryFile& tmpFile = hashTempMap[filehash.get()];
			if (!tmpFile.open()) {
				printGuiMessage("Unable to create temporary file for serialization data!", MSGS::FAIL);
			} else {
				// store the tracking data of the previously selected algorithm
				// in the temporary file.
				storeTrackingData(tmpFile.fileName().toStdString());
			}
		}
		_tracker.reset();
	}

	if (trackingAlg != Algorithms::NoTracking)
	{
		_tracker = Algorithms::Registry::getInstance().make_new_tracker(trackingAlg, _settings, this);
		assert(_tracker);

		// init tracking Alg
		_tracker->setCurrentFrameNumber(static_cast<int>(_currentFrame));
        _tracker->setmaxFrameNumber(_trackingThread->getVideoLength()-1);
		connectTrackingAlg(_tracker);
         _tracker->setCurrentVideoMode(_videoMode);

		// now we try to find a temporary file that contains previously
		// stored tracking data for the new tracking algorithm and the
		// currently opened file.
		if (filehash) {
			const auto& hash_tmp_it = _serializationTmpFileMap.find(trackingAlg);
			if (hash_tmp_it != _serializationTmpFileMap.end()) {
				map_hash_temp_t& hashTempMap = (*hash_tmp_it).second;
				// find the previously temporary file
				const auto& tmpfile_it = hashTempMap.find(filehash.get());
				if (tmpfile_it != hashTempMap.end()) {
					QTemporaryFile& file = (*tmpfile_it).second;
					// open file and restore tracking data
					if (!file.open()) printGuiMessage("Unable to load temporary file with serialization data!", MSGS::FAIL);
					loadTrackingData(file.fileName().toStdString());
				}
			}
		}
	}

	_settings.setParam(TRACKERPARAM::SELECTED_TRACKER, Algorithms::Registry::getInstance().stringByType().at(trackingAlg));

	ui.groupBox_params->repaint();
	ui.groupBox_tools->repaint();
	emit changeTrackingAlg(_tracker);
}

void Gui::connectTrackingAlg(std::shared_ptr<TrackingAlgorithm> tracker)
{	
	if(_tracker)
	{
		QObject::connect(tracker.get(), SIGNAL(notifyGUI(std::string, MSGS::MTYPE)),
			this, SLOT(printGuiMessage(std::string, MSGS::MTYPE)));
        QObject::connect( tracker.get(), SIGNAL( update() ),
            ui.videoView, SLOT( update() ));
		QObject::connect(tracker.get(), SIGNAL( forceTracking() ),
			_trackingThread.get(), SLOT( doTrackingAndUpdateScreen() ));
		QObject::connect(_trackingThread.get(), SIGNAL( newFrameNumber(int) ),
			tracker.get(), SLOT( setCurrentFrameNumber(int) ));
		QObject::connect(ui.videoView, SIGNAL(reportZoomLevel(float)),
			tracker.get(), SLOT(setZoomLevel(float)));
		QObject::connect(_trackingThread.get(), &TrackingThread::trackingSequenceDone,
						 _tracker.get(), &TrackingAlgorithm::setCurrentImage);
		QObject::connect(tracker.get(), &TrackingAlgorithm::registerViews,
						 this, &Gui::setViews);
        QObject::connect(tracker.get(), &TrackingAlgorithm::pausePlayback,
                         this, &Gui::videoPause);
        QObject::connect(tracker.get(), &TrackingAlgorithm::jumpToFrame,
                         this, &Gui::changeCurrentFramebySignal);

		_tracker->postConnect();

		_tracker->setZoomLevel(ui.videoView->getCurrentZoomLevel());
		try
		{
			_paramsWidget = _tracker->getParamsWidget();
			_vboxParams->addWidget(_paramsWidget.get());
			_toolsWidget = _tracker->getToolsWidget();
			_vboxTools->addWidget(_toolsWidget.get());
		}
		catch(std::exception&)
		{
			emit printGuiMessage("cannot create UI elements for selected algorithm", MSGS::FAIL);
		}
	}
	ui.videoView->update();
}

void Gui::takeScreenshot()
{
    QString filename = "";
    const std::chrono::system_clock::time_point p = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(p);
    std::string dateTime = std::ctime(&t);
    //ctime adds a newline to the string due to historical reasons
    dateTime = dateTime.substr(0, dateTime.size() - 1);
    filename.append("/screenshot_").append(QString::fromStdString(dateTime)).append(".png");
    QString filepath = QFileDialog::getSaveFileName(this, tr("Save File"), "/home/"+filename , tr("Images (*.png)"));
	ui.videoView->takeScreenshot(filepath);
}

void Gui::viewIndexChanged(int index)
{
	if (_isPanZoomMode) ui.button_panZoom->click();
	if (index == 0) {
		emit changeSelectedView(TrackingAlgorithm::OriginalView);
	} else {
		emit changeSelectedView(_availableViews.at(index - 1));
	}
}

void Gui::switchPanZoomMode()
{
	_isPanZoomMode = !_isPanZoomMode;
	ui.videoView->setPanZoomMode(_isPanZoomMode);
}

void Gui::displayFileName(QString filename)
{
	_filename = QFileInfo(filename).fileName();
	statusBar()->showMessage(_filename);
	setWindowTitle("BioTracker [" + _filename + "]");
}

void Gui::resetViews()
{
	ui.comboBoxSelectedView->setCurrentIndex(0);
	for (int index = ui.comboBoxSelectedView->count() - 1; index > 0; --index) {
		ui.comboBoxSelectedView->removeItem(index);
	}
}

void Gui::setViews(std::vector<TrackingAlgorithm::View> views)
{
	resetViews();
	for (TrackingAlgorithm::View const& view : views) {
		ui.comboBoxSelectedView->addItem(QString::fromStdString(view.name));
	}

	_availableViews = std::move(views);
}
void Gui::setVideoMode(GUIPARAM::VideoMode vidMode)
{
    _videoMode = vidMode;
    if(_tracker)
        _tracker.get()->setCurrentVideoMode(vidMode);
}