#ifndef CONTROLLERPLAYER_H
#define CONTROLLERPLAYER_H

#include "QThread"
#include "Interfaces/icontroller.h"
#include "Model/BioTracker3Player.h"


class ControllerPlayer : public IController
{
    Q_OBJECT
public:
    ControllerPlayer(QObject *parent = 0, IBioTrackerContext *context = 0, ENUMS::CONTROLLERTYPE ctr = ENUMS::CONTROLLERTYPE::NO_CTR);

    void loadImageStream(QString str);

    void nextFrame();
    void prevFrame();
    void play();
    void stop();
    void pause();


    void changeImageView(QString str);

    // IController interface
public:
    void connectViewToMainWindow(IController *controller) override;
    void connectToOtherController(IController *controller) override;
    void callAnOtherController() override;

protected:
    void createModel() override;
    void createView() override;
    void connectModelController() override;
    void connectModelView() override;

Q_SIGNALS:
  void emitLoadImageStream(QString str);
  void emitNextFrameCommand();
  void emitPrevFrameCommand();
  void emitPlayCommand();
  void emitStopCommand();
  void emitPauseCommand();

  void emitRunPlayerOperation();


public Q_SLOTS:
  void handlePlayerResult();
  void handleNewCvMat(cv::Mat mat);

private:
    QThread *m_PlayerThread;

    IModel *m_Player;
};

#endif // CONTROLLERPLAYER_H