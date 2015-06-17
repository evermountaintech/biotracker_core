#pragma once

#include <QIcon>
#include <QWidget>

#include "source/core/Facade.h"
#include "source/gui/ui_VideoControlWidget.h"

namespace BioTracker {
namespace Gui {

class VideoControlWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoControlWidget(QWidget *parent, Core::Facade &facade, VideoView* videoView);

    Ui::VideoControlWidget& getUi() { return m_ui; }

private:
    Ui::VideoControlWidget m_ui;
    Core::Facade& m_facade;
    VideoView* m_videoView;

    QIcon m_iconPause;
    QIcon m_iconPlay;

    bool m_isPanZoomMode;

    void updateWidgets();
    void initConnects();
    void initShortcuts();

private slots:
    void playPause();
    void setFrame(const int frame);
    void nextFrame();
    void previousFrame();
    void changeCurrentFrameByEdit();
    void takeScreenshot();
    void switchPanZoomMode();
};

}
}

