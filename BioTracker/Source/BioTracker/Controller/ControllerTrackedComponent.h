#ifndef CONTROLLERTRACKEDCOMPONENT_H
#define CONTROLLERTRACKEDCOMPONENT_H

#include "Interfaces/IController/IController.h"

class ControllerTrackedComponent : public IController
{
public:
    ControllerTrackedComponent(QObject *parent = 0, IBioTrackerContext *context = 0, ENUMS::CONTROLLERTYPE ctr = ENUMS::CONTROLLERTYPE::COMPONENT);

    // IController interface
protected:
    void createModel() override;
    void createView() override;
    void connectModelToController() override;
    void connectControllerToController() override;
};

#endif // CONTROLLERTRACKEDCOMPONENT_H
