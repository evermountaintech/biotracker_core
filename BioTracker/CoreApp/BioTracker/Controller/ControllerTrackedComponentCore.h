#ifndef CONTROLLERTRACKEDCOMPONENTCORE_H
#define CONTROLLERTRACKEDCOMPONENTCORE_H

#include "Interfaces/IController/IController.h"
#include "Interfaces\IModel\IModelTrackedTrajectory.h"

class ControllerTrackedComponentCore : public IController
{
	Q_OBJECT
	public:
		ControllerTrackedComponentCore(QObject *parent = 0, IBioTrackerContext *context = 0, ENUMS::CONTROLLERTYPE ctr = ENUMS::CONTROLLERTYPE::TRACKEDCOMPONENTCORE);

		IView *getTrackingElementsWidgetCore();

		void addModel(IModel* model);

		IModel* getCoreParameter();

	signals:
		// signal to ctrPlugin to remove trajectory
		void emitRemoveTrajectory(IModelTrackedTrajectory* trajectory);
		// signal to ctrPlugin to add trajectory
		void emitAddTrajectory(QPoint pos);

		void emitMoveElement(IModelTrackedTrajectory* trajectory, QPoint pos);

		void emitSwapIds(IModelTrackedTrajectory* trajectory0, IModelTrackedTrajectory* trajectory1);

		void emitCoreParameterView(IView* coreParameterView);

	public Q_SLOTS:
		//this slot gets triggered when the tracking is done, so the core-view updates
		void receiveTrackingOperationDone(uint framenumber);
		//gets triggered when plugin sends permissions
		void setCorePermission(std::pair<ENUMS::COREPERMISSIONS, bool> permission);

		//gets triggered when shape-trajectory is getting removed
		void receiveRemoveTrajectory(IModelTrackedTrajectory* trajectory);

		void receiveAddTrajectory(QPoint pos);

		void receiveMoveElement(IModelTrackedTrajectory* trajectory, QPoint pos);

		void receiveSwapIds(IModelTrackedTrajectory* trajectory0, IModelTrackedTrajectory* trajectory1);

		// IController interface
	protected:
		void createModel() override;
		void createView() override;
		void connectModelToController() override;
		void connectControllerToController() override;

		IView* m_parameterView;
		IModel* m_coreParameterModel;
};

#endif // CONTROLLERTRACKEDCOMPONENTCORE_H