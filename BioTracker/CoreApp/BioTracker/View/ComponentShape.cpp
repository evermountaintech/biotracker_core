#include "ComponentShape.h"
#include "QBrush"
#include "QPainter"
#include "QMenu"
#include "QAction"
#include "QGraphicsScene"
#include "QGraphicsSceneContextMenuEvent"
#include "QDebug"
#include "QColorDialog"
#include "qwidgetaction.h"
#include "qlabel.h"
#include "assert.h"

ComponentShape::ComponentShape(QGraphicsObject* parent, IModelTrackedTrajectory* trajectory, int id):
	QGraphicsObject(parent), m_trajectory(trajectory), m_id(id), m_parent(parent)
{
	setData(0, m_id);

	m_polygons = QList<QPolygonF>();
	m_useDefaultDimensions = true;
	m_penWidth = 0;
	m_penStyle = Qt::SolidLine;
	m_marked = false;
	m_pMovable = true;
	m_pRemovable = true;
	m_pSwappable = true;
	m_currentFramenumber = 0;
	m_rotation = 0;

	m_tracingLayer = new QGraphicsRectItem();
	this->scene()->addItem(m_tracingLayer);

	setFlag(ItemIsMovable);
	setFlag(ItemIsSelectable);
	setFlag(ItemSendsGeometryChanges);

	setAcceptedMouseButtons(Qt::LeftButton);

	setVisible(true);
}

ComponentShape::~ComponentShape() {
    delete m_tracingLayer;
}

QRectF ComponentShape::boundingRect() const
{
	if (this->data(1) == "ellipse" || this->data(1) == "point" || this->data(1) == "rectangle") {
		return QRectF(0, 0, m_w, m_h);
	}
	else if (this->data(1) == "polygon") {
		//outer polygon bounding rect
		return m_polygons[0].boundingRect();
	}
	else {
		qDebug() << "Could not create a bounding rect for current track" << m_id;
		assert(0);
	}

}

QPainterPath ComponentShape::shape() const
{
	QPainterPath path;
	if (this->data(1) == "ellipse" || this->data(1) == "point") {
		path.addEllipse(0, 0, m_w, m_h);
	}
	else if (this->data(1) == "recatangle") {
		path.addRect(0, 0, m_w, m_h);
	}
	else if (this->data(1) == "polygon") {
		//outer polygon
		path.addPolygon(m_polygons[0]);
	}
	else {
		qDebug() << "Could not create a shape (interaction area) for current track" << m_id;
		assert(0);
	}
	return path;
}

void ComponentShape::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

   
    if (m_currentFramenumber < 0)
        return;
	//Antialiasing
	if (m_antialiasing) {
		painter->setRenderHint(QPainter::Antialiasing);
	}

	//check if scene is set
	if (!(this->scene())) {
		qDebug() << "componentscene is null\n";
	}

	QPen pen = QPen(m_penColor, m_penWidth, m_penStyle);
	QBrush brush = QBrush(m_brushColor);
	painter->setPen(pen);
	painter->setBrush(brush);

	//TODO enums for types?

	//draw ellipse
	if (this->data(1) == "ellipse") {
		QRectF ellipse = QRectF(0, 0, m_w, m_h);
		painter->drawEllipse(ellipse);
	}

	//draw rectangle
	if (this->data(1) == "rectangle") {
		QRect rectangle = QRect(0, 0, m_w, m_h);
		painter->drawRect(rectangle);
	}

	//draw point
	//TODO how to visualize this the best way
	//a point should always have the same height and width
	if (this->data(1) == "point") {
		QRectF ellipse = QRectF(0, 0, m_w, m_h);
		painter->drawEllipse(ellipse);
	}

	//draw polygon
	if (this->data(1) == "polygon") {
		foreach(QPolygonF polygonF, m_polygons) {
			painter->drawPolygon(polygonF);
		}
	}

	// draw angleLine
	if (m_orientationLine) {
		QLineF angleline;
		angleline.setP1(QPointF(m_w / 2, m_h / 2));
		if (m_h > m_w) {
			angleline.setAngle(-90);
		}
		else {
			angleline.setAngle(0);
		}
		//TODO length changable
		angleline.setLength(100);
		painter->drawLine(angleline);
	}
}

bool ComponentShape::advance()
{
	return false;
}

//put this in updateAttributes
bool ComponentShape::updatePosition(uint framenumber)
{
	//printf("update a shape\n");
	m_currentFramenumber = framenumber;

	if (!m_trajectory) {
		//printf("trajectory not existant, delete child %i...\n", m_id);
		delete this;
		return false;
	}
	if (m_trajectory->size() != 0 && m_trajectory->getValid()) {

		//update width and height for the latest tracked component
		updateAttributes(); 

		//for each point-like (point, rectangle, ellipse)
		IModelTrackedPoint* pointLike = dynamic_cast<IModelTrackedPoint*>(m_trajectory->getChild(framenumber));
		if (pointLike) {
			// if component and traj valid -> show and update
			if (pointLike->getValid() && m_trajectory->getValid()) {
				this->setPos(pointLike->getXpx() - m_w/2, pointLike->getYpx() - m_h/2);

				//create tracers
				trace();

				this->show();
				update();

				//qDebug() << "shape is updated to:" << pos();

				//printf("shape updated\n");
			}
			// else hide shape
			else {
				this->hide();
				update();
			}
			return true;
		}
		else {
			qDebug() << "no tracked component in this trajectory in frame: " << framenumber;
			return true;
		}
		//for each polygon
		IModelTrackedPolygon* polygons = dynamic_cast<IModelTrackedPolygon*>(m_trajectory->getChild(framenumber));
		if (polygons) {

		}
	}
	else {
		//printf("trajectory empty, delete child%i...\n", m_id);
		delete this;
		return false;
	}

}
//enables possibility for plugin to change width, height, rotation
//TODO for polygons
void ComponentShape::updateAttributes()
{
	prepareGeometryChange();
	IModelTrackedPoint* pointLike = dynamic_cast<IModelTrackedPoint*>(m_trajectory->getChild(m_currentFramenumber));
	if (pointLike) {
		//update width and height ore use defaults
		if (m_useDefaultDimensions) {
			if (pointLike->getW() > 0) {
				m_w = pointLike->getW();
			}
			else { m_w = m_wDefault; }

			if (pointLike->getH() > 0) {
				m_h = pointLike->getH();
			}
			else { m_h = m_hDefault; }
		}

		//update rotation
		this->setTransformOriginPoint(m_w / 2, m_h / 2);
		if (m_h > m_w) {
			m_rotation = -90 - pointLike->getDeg();
			this->setRotation(m_rotation);
		}
		else {
			m_rotation = -pointLike->getDeg();
			this->setRotation(m_rotation);
		}
	}
}


//TODO is ugly -> make prettier!
void ComponentShape::trace()
{
	//TRACING

	IModelTrackedPoint* currentChild = dynamic_cast<IModelTrackedPoint*>(m_trajectory->getChild(m_currentFramenumber));
	if (!currentChild)
		return;

	QPointF currentPoint = QPointF(currentChild->getXpx(), currentChild->getYpx());

	m_tracingLayer->setPos(this->pos());
	//m_tracingLayer->setTransformOriginPoint(m_w / 2, m_h / 2);
	//m_tracingLayer->setRotation(-m_rotation);
	m_tracingLayer->setFlag(QGraphicsItem::ItemHasNoContents);
	m_tracingLayer->show();

	// really unefficient to flush each time
	//flush tracing shape history
	//TODO make this more efficient(e.g. delete last element and append new element)
/*	while (m_tracingHistoryShapes.size() > 0) {
		m_tracingHistoryShapes[0]->hide();
		m_tracingHistoryShapes.removeAt(0);
	}*/	
	while (m_tracingLayer->childItems().size() > 0) {
		delete m_tracingLayer->childItems()[0];
	}

	if (m_trajectory->size() != 0 && m_tracingLength > 0 && !(m_tracingStyle == "None")) {


		QPointF lastPointDifference = QPointF(0, 0) + QPointF(m_h / 2, m_w / 2);

		for (int i = 1; i <= m_tracingLength; i += m_tracingSteps) {
			
			//i cannot be larger than m_currentFramenumber -> cannot be in the future / index out of bounds possible
			if (i >= m_currentFramenumber){
				continue;
			}
			
			IModelTrackedPoint* historyChild = dynamic_cast<IModelTrackedPoint*>(m_trajectory->getChild(m_currentFramenumber - i));
			if (historyChild && historyChild->getValid()) {

				//positioning
				QPointF historyPoint = QPointF(historyChild->getXpx(), historyChild->getYpx());
				QPointF historyPointDifference = historyPoint - currentPoint;
				QPointF adjustedHistoryPointDifference = historyPointDifference + QPointF(m_w / 2, m_h / 2);

				if (m_tracingStyle == "Shape") {

					//time degradation colors
					QPen timeDegradationPen = QPen(m_penColor, m_penWidth, m_penStyle);
					QBrush timeDegradationBrush = QBrush(m_brushColor);

					if (m_tracingTimeDegradation == "Transparency") {
						QColor timeDegradationPenColor = QColor(m_penColor.red(), m_penColor.green(), m_penColor.blue(), (200.0f - (200.0f / (float)m_tracingLength) * i) + 30);
						timeDegradationPen = QPen(timeDegradationPenColor, m_penWidth, Qt::SolidLine);

						QColor timeDegradationBrushColor = QColor(m_brushColor.red(), m_brushColor.green(), m_brushColor.blue(), (200.0f - (200.0f / (float)m_tracingLength) * i) + 30);
						timeDegradationBrush = QBrush(timeDegradationBrushColor);

					}
					else if (m_tracingTimeDegradation == "False color") {
						float hue = (240.0f - ((240.0f / (float)m_tracingLength) * i));
						QColor timeDegradationPenColor = QColor::fromHsv((int)hue, 255.0f, 255.0f);
						QColor timeDegradationBrushColor = QColor::fromHsv((int)hue, 255.0f, 255.0f);
						timeDegradationPen = QPen(m_penColor, m_penWidth, m_penStyle);
						timeDegradationBrush = QBrush(timeDegradationBrushColor);
					}

					//orientation line
					if (m_tracingOrientationLine) {
						QLineF orientationLine = QLineF();
						orientationLine.setP1(adjustedHistoryPointDifference);
						orientationLine.setAngle(historyChild->getDeg());
						orientationLine.setLength(15);

						QGraphicsLineItem* orientationItem = new QGraphicsLineItem(orientationLine, m_tracingLayer);
					}

					if (this->data(1) == "point") {
						QGraphicsEllipseItem* tracePoint = new QGraphicsEllipseItem(m_tracingLayer);
						tracePoint->setPos(adjustedHistoryPointDifference);
						tracePoint->setRect(QRect(-m_w * m_tracerProportions / 2, -m_h * m_tracerProportions / 2, m_w * m_tracerProportions, m_h * m_tracerProportions));
						//tracer orientation
						float tracerOrientation;
						if (m_h > m_w) {tracerOrientation = -90 - historyChild->getDeg();}
						else {tracerOrientation = -historyChild->getDeg();}
						tracePoint->setRotation(tracerOrientation);
						//set colors
						tracePoint->setPen(timeDegradationPen);
						tracePoint->setBrush(timeDegradationBrush);
						tracePoint->setFlag(QGraphicsItem::ItemIgnoresTransformations);
					}
					else if (this->data(1) == "ellipse") {
						QGraphicsEllipseItem* traceEllipse = new QGraphicsEllipseItem(m_tracingLayer);
						traceEllipse->setPos(adjustedHistoryPointDifference);
						traceEllipse->setRect(QRect(-m_w * m_tracerProportions / 2, -m_h * m_tracerProportions / 2, m_w * m_tracerProportions, m_h * m_tracerProportions));
						//set orientation
						float tracerOrientation;
						if (m_h > m_w) {tracerOrientation = -90 - historyChild->getDeg();}
						else {tracerOrientation = -historyChild->getDeg();}
						traceEllipse->setRotation(tracerOrientation);
						//set colors
						traceEllipse->setPen(timeDegradationPen);
						traceEllipse->setBrush(timeDegradationBrush);
					}
					else if (this->data(1) == "rectangle") {
						QGraphicsEllipseItem* traceRect = new QGraphicsEllipseItem(m_tracingLayer);
						traceRect->setPos(adjustedHistoryPointDifference);
						traceRect->setRect(QRect(-m_w * m_tracerProportions / 2, -m_h * m_tracerProportions / 2, m_w * m_tracerProportions, m_h * m_tracerProportions));
						//set orientation
						float tracerOrientation;
						if (m_h > m_w) {tracerOrientation = -90 - historyChild->getDeg();}
						else {tracerOrientation = -historyChild->getDeg();}
						traceRect->setRotation(tracerOrientation);
						//set colors
						traceRect->setPen(timeDegradationPen);
						traceRect->setBrush(timeDegradationBrush);
					}
					//TODO polygons
				}

				//PATH
				else if (m_tracingStyle == "Path") {

					QLineF base = QLineF(adjustedHistoryPointDifference, lastPointDifference);
					QGraphicsLineItem* lineItem = new QGraphicsLineItem(base, m_tracingLayer);
					lineItem->setPen(QPen(m_penColor, m_penWidth, m_penStyle));

					lastPointDifference = adjustedHistoryPointDifference;
				}
				//ARROWPATH
				else if (m_tracingStyle == "ArrowPath") {
					QLineF base = QLineF(adjustedHistoryPointDifference, lastPointDifference);

					QLineF arm0 = base.normalVector();
					arm0.setLength(10);
					arm0.setAngle(base.angle() -160);

					QLineF arm1 = base.normalVector();
					arm1.setLength(10);
					arm1.setAngle(base.angle() - 200);

					QGraphicsLineItem* baseItem = new QGraphicsLineItem(base, m_tracingLayer);
					baseItem->setPen(QPen(m_penColor, m_penWidth, m_penStyle));
					QGraphicsLineItem* arm0Item = new QGraphicsLineItem(arm0, m_tracingLayer);
					arm0Item->setPen(QPen(m_penColor, m_penWidth, m_penStyle));
					QGraphicsLineItem* arm1Item = new QGraphicsLineItem(arm1, m_tracingLayer);
					arm1Item->setPen(QPen(m_penColor, m_penWidth, m_penStyle));

					lastPointDifference = adjustedHistoryPointDifference;
				}
			}
			
		}
	}
}

IModelTrackedTrajectory * ComponentShape::getTrajectory()
{
	return m_trajectory;
}

void ComponentShape::setPermission(std::pair<ENUMS::COREPERMISSIONS, bool> permission)
{
	m_permissions.insert(permission);


	switch (permission.first)
	{
		case ENUMS::COREPERMISSIONS::COMPONENTMOVE:
			m_pMovable = permission.second;
			this->setFlag(ItemIsMovable, permission.second);
			break;
		case ENUMS::COREPERMISSIONS::COMPONENTREMOVE:
			m_pRemovable = permission.second;
			break;
		case ENUMS::COREPERMISSIONS::COMPONENTSWAP:
			m_pSwappable = permission.second;
	}

	//qDebug() << "shape permission " << permission.first << "set to " << permission.second;
}

int ComponentShape::getId()
{
	return m_id;
}

bool ComponentShape::isSwappable()
{
	return m_permissions[ENUMS::COREPERMISSIONS::COMPONENTSWAP];
}

bool ComponentShape::isRemovable()
{
	return m_pRemovable;
}

void ComponentShape::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
	//qDebug() << "shape get dragged at:" << pos();

	m_mousePressPos = pos().toPoint();
	//qDebug()<< "PRESS" << m_mousePressPos;

	if (event->button() == Qt::LeftButton) {
		// handle left mouse button here
		setCursor(Qt::ClosedHandCursor);
		m_dragged = true;
		update();
	}
	//pass on
	QGraphicsItem::mousePressEvent(event);
}

void ComponentShape::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
	if (event->button() == Qt::LeftButton) {
		setCursor(Qt::ArrowCursor);
		m_dragged = false;
		//qDebug() << "DROP " << this->getId() << ": " << pos();
		
		// signal new position to controller
		//emitMoveElement(m_trajectory, pos().toPoint());

		//broadcast move so other selected elements get moved too
		// TODO? maybe unconventional and slow but couldn't find another way; Dropevents in view and dropevents in shape didn't seem to work
		broadcastMove();

		update();
	}
	//pass on
	QGraphicsItem::mouseReleaseEvent(event);

}

void ComponentShape::mouseMoveEvent(QGraphicsSceneMouseEvent * event) {
	//qDebug() << "shape gets moved to:" << pos();
	QGraphicsItem::mouseMoveEvent(event);
}

QVariant ComponentShape::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged && scene()) {
		if (this->isSelected()) {
			m_penColorLast = m_penColor;
			m_penColor = Qt::red;
			m_penStyle = Qt::DashLine;
			trace();
			update();
		}
		else {
			m_penColor = m_penColorLast;
			m_penStyle = Qt::SolidLine;
			trace();
			update();

		}
	}
	return QGraphicsItem::itemChange(change, value);
}

void ComponentShape::contextMenuEvent(QGraphicsSceneContextMenuEvent * event)
{
	QMenu menu;
	
	// create the info box
	QWidgetAction* infoBox = new QWidgetAction(this);

	QString info = QString("ID: ");
	info.append(QString::number(m_id));
	QLabel* infoLabel = new QLabel(info);
	infoLabel->setWordWrap(true);
	infoLabel->setStyleSheet("QLabel {font-weight: bold; text-align: center}");
	infoLabel->setAlignment(Qt::AlignCenter);
	infoBox->setDefaultWidget(infoLabel);
	menu.addAction(infoBox);
	menu.addSeparator();
	QAction *changeBrushColorAction = menu.addAction("Change fill color",dynamic_cast<ComponentShape*>(this),SLOT(changeBrushColor()));
	QAction *changePenColorAction = menu.addAction("Change border color", dynamic_cast<ComponentShape*>(this), SLOT(changePenColor()) );
	QAction *showProperties = menu.addSeparator();
	QAction *removeTrackAction = menu.addAction("Remove track", dynamic_cast<ComponentShape*>(this), SLOT(removeTrack()));
	QAction *removeTrackEntityAction = menu.addAction("Remove track entity", dynamic_cast<ComponentShape*>(this), SLOT(removeTrackEntity()));
	if (!m_pRemovable) { 
		removeTrackAction->setEnabled(false);
		removeTrackEntityAction->setEnabled(false);
	}

	QAction *markAction = menu.addAction("Mark", dynamic_cast<ComponentShape*>(this), SLOT(markShape()));
	QAction *unmarkAction = menu.addAction("Unmark", dynamic_cast<ComponentShape*>(this), SLOT(unmarkShape()));
	if (m_marked) {
		markAction->setEnabled(false);
		unmarkAction->setEnabled(true);
	}
	else {
		markAction->setEnabled(true);
		unmarkAction->setEnabled(false);
	}
	
	QAction *selectedAction = menu.exec(event->screenPos());
}

//SLOTS

void ComponentShape::changeBrushColor()
{
	this->m_brushColor = QColorDialog::getColor();
	trace();
	update();
}

void ComponentShape::changePenColor()
{
	QColor newColor = QColorDialog::getColor();

	if (!isSelected()) {
		m_penColorLast = m_penColor;
		m_penColor = newColor;

		trace();
		update();
	}
	else {
		m_penColorLast = newColor;
	}

}

void ComponentShape::changeBrushColor(QColor color)
{
	m_brushColor = color;

	trace();
	update();
}

void ComponentShape::changePenColor(QColor color)
{
	if (!isSelected()) {
		m_penColorLast = m_penColor;
		m_penColor = color;
		trace();
		update();
	}
	else {
		m_penColorLast = color;
	}
}

bool ComponentShape::removeTrack()
{
	if (m_pRemovable) {
		qDebug() << "Removing track...";

		//emit to set trajectory invalid 
		Q_EMIT emitRemoveTrajectory(m_trajectory);
		//hide this shape
		this->hide();
		this->m_tracingLayer->hide();
	}
	else {
		qDebug() << "track is not removable";
	}
	return m_pRemovable;
}

bool ComponentShape::removeTrackEntity()
{
	if (m_pRemovable) {
		qDebug() << "Removing track entity...";

		//emit to set trajectory invalid 
		Q_EMIT emitRemoveTrackEntity(m_trajectory);
		//hide this shape
		this->hide();
	}
	else {
		qDebug() << "trackentity is not removable";
	}
	return m_pRemovable;
}

void ComponentShape::markShape(int penwidth)
{	
	if (!m_marked) {
		m_penWidth = 3;
	}
	else {
		m_penWidth = penwidth;
	}
	m_marked = true;
	trace();
	update();
}

void ComponentShape::unmarkShape()
{
	m_marked = false;
	m_penWidth = 0;

	trace();
	update();
}

void ComponentShape::receiveTracingLength(int tracingLength)
{
	m_tracingLength = tracingLength;
	trace();
	update();
}

void ComponentShape::receiveTracingStyle(QString style)
{
	m_tracingStyle = style;
	trace();
	update();
}

void ComponentShape::receiveTracingSteps(int steps)
{
	m_tracingSteps = steps;
	trace();
	update();
}

void ComponentShape::receiveTracingTimeDegradation(QString timeDegradation)
{
	m_tracingTimeDegradation = timeDegradation;
	trace();
	update();
}

void ComponentShape::receiveTracerProportions(float proportion)
{
	m_tracerProportions = proportion;
	trace();
	update();
}

void ComponentShape::receiveTracerOrientationLine(bool toggle)
{
	m_tracingOrientationLine = toggle;
	trace();
	update();
}

void ComponentShape::receiveAntialiasing(bool toggle)
{
	m_antialiasing = toggle;
	trace();
	update();
}

void ComponentShape::receiveDimensions(int width, int height)
{
	m_useDefaultDimensions = false;
	m_w = width;
	m_h = height;
	updatePosition(m_currentFramenumber);
	trace();
	update();
}

void ComponentShape::setDimensionsToDefault()
{
	m_useDefaultDimensions = true;
	m_w = m_wDefault;
	m_h = m_hDefault;
	updatePosition(m_currentFramenumber);
	trace();
	update();
}

void ComponentShape::receiveToggleOrientationLine(bool toggle)
{
	m_orientationLine = toggle;
	trace();
	update();
}
//set members from core params
void ComponentShape::setMembers(CoreParameter* coreParams)
{
	m_antialiasing = coreParams->m_antialiasing;

	m_tracingStyle = coreParams->m_tracingStyle;
	m_tracingTimeDegradation = coreParams->m_tracingTimeDegradation;
	m_tracingLength = coreParams->m_tracingHistory;
	m_tracingSteps = coreParams->m_tracingSteps;
	m_tracerProportions = coreParams->m_tracerProportions;
	m_tracingOrientationLine = coreParams->m_tracerOrientationLine;
	m_orientationLine = coreParams->m_trackOrientationLine;

	m_brushColor = *(coreParams->m_colorBrush);
	m_penColor = *(coreParams->m_colorBorder);


	// set dimensions and default dimensions
	if (coreParams->m_trackWidth > 0) {
		m_w = coreParams->m_trackWidth;
		m_wDefault = coreParams->m_trackWidth;
	}
	if (coreParams->m_trackHeight > 0) {
		m_h = coreParams->m_trackHeight;
		m_hDefault = coreParams->m_trackHeight;
	}

	update();
}
