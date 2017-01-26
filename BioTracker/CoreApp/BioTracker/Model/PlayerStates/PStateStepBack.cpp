#include "PStateStepBack.h"
#include "Model/MediaPlayer.h"

PStateStepBack::PStateStepBack(MediaPlayer *player,
                               std::shared_ptr<BioTracker::Core::ImageStream> imageStream) :
    IPlayerState(player, imageStream)  {

}

void PStateStepBack::operate() {

    m_Play = true;
    m_Forw = true;
    m_Stop = true;
    m_Paus = false;


    if (m_ImageStream->previousFrame()) {
        m_Mat =m_ImageStream->currentFrame();
        m_FrameNumber = m_ImageStream->currentFrameNumber();
    }

    bool stateBack = false;
    if (m_FrameNumber <= 0) {
        stateBack = false;
    } else {
        stateBack = true;
    }

    m_Back = stateBack;

//    Q_EMIT emitStateDone();

    m_Player->setNextState(IPlayerState::STATE_WAIT);

}