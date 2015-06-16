#ifndef BioTracker_H
#define BioTracker_H

#include <memory>

#include <QtWidgets/QMainWindow>
#include <QtCore/QString>
#include <QTemporaryFile>
#include <QShortcut>

#include "source/core/settings/ParamNames.h"
#include "source/core/Registry.h"
#include "source/gui/ui_Gui.h"

Q_DECLARE_METATYPE(cv::Mat)
class TrackingThread;
class VideoView;
class TrackingAlgorithm;
class Settings;

class Gui: public QMainWindow
{
	Q_OBJECT


public:    
	Gui(Settings &settings, QWidget *parent = nullptr,  Qt::WindowFlags flags = 0);
	~Gui();


public slots:
	// open file browser for video in/out
	void browseVideo();
	// open file browser for picture in/out
	void browsePicture();
	// load previously stored tracking data
	void loadTrackingDataTriggered(bool checked = false);
	// store current tracking data
	void storeTrackingDataTriggered(bool checked = false);
	// exit application
	void exit();
	//checks current state (stopped,paused or playing)
	//and then sends appropriate signal to tracking thread
	void runCapture();
	// stops video thread
	void stopCapture();
	// next frame 
	void stepCaptureForward();
	// previous frame
	void stepCaptureBackward();
	//pause video thread;
	void pauseCapture();
	//change video playback speed
	void changeFps(int fps);
	//different tracking algorithm was selected
	void trackingAlgChanged(Algorithms::Type trackingAlg);
	void trackingAlgChanged(QString trackingAlgStr);
	//switch pan&zoom mode
	void switchPanZoomMode();

	// SLOTS FOR TRACKING THREAD: 	
	void updateFrameNumber(int frameNumber);
    /**
      * @brief will send picture to glContainer (VideoView) which then loads it
      *        into graphics memory
      * @param image the original picture without any modifications
      *
    */
	void drawImage(cv::Mat image);
	void showFps(double fps);
	void invalidFile();
    /**
     * @brief displayFileName will display source url
     * in the notifications tool box
     * @param filename url to video or picture source
     */
	void displayFileName(QString filename);

    /**
     * @brief resetViews will empty view list,
     * which were set by a tracking algorithm
     */
	void resetViews();

    /**
     * @brief setViews will set additional views supported by tracking alg.
     *        (standard: only one view named "original")
     * @param views list of views Tracking algorrithm can switch between
     */
	void setViews(std::vector<TrackingAlgorithm::View> views);

	/**
	 * Print proivided message to the GUI message area.
	 * @param message the message to print.
	 */
	void printGuiMessage(std::string message, MSGS::MTYPE mType = MSGS::MTYPE::NOTIFICATION);


    // methods to change position in picture series, or video
    //--------------------------------Start-------------------------------------
	void changeCurrentFramebySlider();
	void changeCurrentFramebySlider(int SliderAction);
	void changeCurrentFramebyEdit();
    void changeCurrentFramebySignal(int frameNumber);
    //---------------------------------End--------------------------------------

	void takeScreenshot();
    /**
     * @brief viewIndexChanged Signal to TrackingAlgorithm
     * with newly selected view
     * @param index number of view
     */
	void viewIndexChanged(int index);

protected:
	bool event(QEvent* event) override;
	void showEvent(QShowEvent *ev) override;

private:
	Ui::BioTrackerClass ui;

    /**
     * @brief _settings contains all saveable parameters
     */
	Settings& _settings;
    /**
     * @brief _trackingThread is the main thread which will send pictures as
     * cv::Mat to tracking Algorithm and to BioTracker::drawImage()
     */
	std::unique_ptr<TrackingThread> _trackingThread;
    GUIPARAM::VideoMode _videoMode;
    void setVideoMode(GUIPARAM::VideoMode vidMode);

	GUIPARAM::MediaType _mediaType;

	bool _isPanZoomMode;
	size_t _currentFrame;
	QIcon _iconPause;
	QIcon _iconPlay;
	QString _filename;

	//disable or enable buttons for video navigating
	void setPlayfieldPaused(bool enabled);
	void initGui();	
	void initConnects();
	void initPlayback();
	void initAlgorithmList();
	void connectTrackingAlg(std::shared_ptr<TrackingAlgorithm> tracker);
    void setPlayfieldEnabled(bool enabled);

	void loadTrackingData(std::string const& filename);
	void storeTrackingData(std::string const& filename);

	void closeEvent(QCloseEvent* event) override;

	std::shared_ptr<TrackingAlgorithm> _tracker;

	typedef std::string filehash;
	typedef std::map<filehash, QTemporaryFile> map_hash_temp_t;
	typedef std::map<Algorithms::Type, map_hash_temp_t> map_type_hashtemp_t;

	map_type_hashtemp_t _serializationTmpFileMap;

	boost::optional<std::vector<std::string> > getOpenFiles() const;
	boost::optional<filehash> getFileHash(std::string const& filename, const size_t numFiles) const;
	std::vector<std::string> getFilenamesFromPaths(std::vector<std::string> const& paths) const;

	//Containers to put in chosen algorithm specific ui stuff
	QVBoxLayout *_vboxParams;
	QVBoxLayout *_vboxTools;
	std::shared_ptr<QWidget> _paramsWidget;
	std::shared_ptr<QWidget> _toolsWidget;

	std::vector<TrackingAlgorithm::View> _availableViews;

private slots:
	void startUp();


signals:
	//signal when main window is initialized after start up
	void window_loaded();

	// tell tracking thread to grab next frame
	void nextFrameReady(bool);

	// tell tracking thread to grab specific frame
	void changeFrame(int);

	//tell Tracker to stop
	void videoStop();

	// tell tracking thread to pause/unpause video
	void videoPause(bool);	

	//tell tracking thread to grab next frame
	void grabNextFrame();

	//tell tracking thread to change playback speed
	void fpsChange(double fps);
	
	//enable max playback speed
	void enableMaxSpeed (bool enabled);

	//change tracking algorithm
	void changeTrackingAlg(std::shared_ptr<TrackingAlgorithm>);

	void changeSelectedView(TrackingAlgorithm::View const& view);
};

#endif // BioTracker_H