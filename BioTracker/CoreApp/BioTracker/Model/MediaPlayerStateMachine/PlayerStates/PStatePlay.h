/****************************************************************************
  **
  ** This file is part of the BioTracker Framework
  ** by Andreas Jörg
  **
  ****************************************************************************/

#ifndef PSTATEPLAY_H
#define PSTATEPLAY_H

#include "IStates/IPlayerState.h"

/**
 * This Stat is active when a video fiel is playing or a camera device is streaming. If the current frame is the last in the ImageStream this state will set STATE_INITIAL as next state.
 * If the current frame is not the last frame in the ImageStream, the next state will be STATE_PLAY.
 */
class PStatePlay : public IPlayerState {
  public:
    PStatePlay(MediaPlayerStateMachine* player, std::shared_ptr<BioTracker::Core::ImageStream> imageStream);

    // IPlayerState interface
  public Q_SLOTS:
    void operate() override;
};

#endif // PSTATEPLAY_H
