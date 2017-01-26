#ifndef TRACKEDELEMENT_H
#define TRACKEDELEMENT_H

#include "QGraphicsItem"
#include "Interfaces/IModel/IModelTrackedComponent.h"
#include "QString"

/**
 * This class is an example of how a Tracked Element could be defined.
 * This class inherits from the IModelTrackedComponent class and is therefor part of the Composite Pattern.
 * This class represents the Leaf class as stated by the GoF.
 * Objects of this class can have a QObject as parent and as it appears in this example have a name.
 */
class TrackedElement : public IModelTrackedComponent
{
    Q_OBJECT

  public:
    TrackedElement(QObject *parent = 0, QString name = "n.a.");

    QString getName();

    void setX(int val);
    void setY(int val);
    void pressed();
    void notPressed();

    int getX();
    int getY();
    bool getPressedStatus();

    // ITrackedComponent interface
public:
    void operate();

private:
    QString name;
    int x;
    int y;

    bool mPressed;
};

#endif // TRACKEDELEMENT_H