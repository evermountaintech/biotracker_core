#include "BeesBookTagMatcher.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <QApplication>

#include "Common.h"
#include "source/tracking/algorithm/algorithms.h"

namespace {
auto _ = Algorithms::Registry::getInstance().register_tracker_type<
    BeesBookTagMatcher>("BeesBook Tag Matcher");

static const cv::Scalar COLOR_BLUE   = cv::Scalar(255, 0, 0);
static const cv::Scalar COLOR_RED    = cv::Scalar(0, 0, 255);
static const cv::Scalar COLOR_GREEN  = cv::Scalar(0, 255, 0);
static const cv::Scalar COLOR_YELLOW = cv::Scalar(0, 255, 255);
}

BeesBookTagMatcher::BeesBookTagMatcher(Settings & settings, QWidget *parent)
	: TrackingAlgorithm(settings, parent)
	, _currentState(State::Ready)
	, _setOnlyOrient(false)
	, _lastMouseEventTime(std::chrono::system_clock::now())
	, _toolWidget(std::make_shared<QWidget>())
	, _paramWidget(std::make_shared<QWidget>())	
	// TESTCODE START
	, _testGrid3d(cv::Point2i(500,500), 53.0 , 0.0, 0.0, 0.0)
	// TESTCODE END
{
	_UiToolWidget.setupUi(_toolWidget.get());
	setNumTags();
	_activeGrid		= std::make_shared<Grid3D>(_testGrid3d);
	_currentState	= State::SetP0;
}

BeesBookTagMatcher::~BeesBookTagMatcher()
{
}

void BeesBookTagMatcher::track(ulong /* frameNumber */, cv::Mat & /* frame */)
{
	_activeGrid.reset();
	setNumTags();
}

void BeesBookTagMatcher::paint(cv::Mat& image)
{
	// TESTCODE START
	_activeGrid->draw(image, 0);
	// TESTCODE END
	//if (!_trackedObjects.empty()) 
	//{
	//	drawSetTags(image);
	//}
	//if (_currentState == State::SetTag) 
	//{
	//	drawOrientation(image, _orient);
	//} 
	//else 
	//	if (_activeGrid)  
	//	{
	//		drawActiveTag(image);
	//	}
}
void BeesBookTagMatcher::reset() {
}

void BeesBookTagMatcher::postLoad()
{
	setNumTags();
}

//check if MOUSE BUTTON IS CLICKED
void BeesBookTagMatcher::mousePressEvent(QMouseEvent * e) 
{
	// position of mouse cursor 
	cv::Point p(e->x(), e->y());

	// left mouse button down:
	// select tag among all visible tags
	// if there is a selected tag: select keypoint
	// if no keypoint selected: compute P2 = _center - p; set space rotation
	// LMB with CTRL: new tag
	// RMB without modifier: store click point temporarily, set rotation mode


	//check if LEFT button is clicked
	if (e->button() == Qt::LeftButton) 
	{
		/*
		//check for SHIFT modifier
		if ( Qt::ShiftModifier == QApplication::keyboardModifiers() )
		{
			if ( _activeGrid )    // The Tag is active and can now be modified
			{
				//if clicked on one of the set points, the point is activated
				if (selectPoint(cv::Point(e->x(), e->y())))
				{
					emit update();
				}
				if (_currentState == State::SetP1)
				{
					_setOnlyOrient = true;
				}
			}
		}*/
		
		/*
		//check for CTRL modifier
		else 
			if ( Qt::ControlModifier == QApplication::keyboardModifiers() )
			{
				if (_currentState == State::Ready) 
				{
					_activeGrid.reset();
					// a new tag is generated
					setTag(cv::Point(e->x(), e->y()));
				}
			}
			else */
			{
				// The Tag is active and can now be modified
				if (_activeGrid) 
				{
					size_t id = _activeGrid->getKeyPointIndex(p);
					const bool indeterminate =
						QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) &&
						QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

					if ((id >= 0) && (id < 12)) // ToDo: use constants
					{
						// change state, so consequent mouse moves do not move the tag when toggling bits
						_currentState = State::SetBit;

						_activeGrid->toggleIdBit(id, indeterminate);
						emit update();
					}	
					else 
						if (id == 13) // P1, ToDo: use constants
						{
							_currentState = State::SetP1;
							_orient.clear();
							_orient.push_back(p);
						}
				} 
				/*
				else 
				{
					selectTag(cv::Point(e->x(), e->y()));
				}*/
			}
	}
	//check if RIGHT button is clicked
	if (e->button() == Qt::RightButton) 
	{
		//check for CTRL modifier
		if (Qt::ControlModifier == QApplication::keyboardModifiers()) 
		{
			removeCurrentActiveTag();
			emit update();
		}
	}
}

//check if pointer MOVES
void BeesBookTagMatcher::mouseMoveEvent(QMouseEvent * e) 
{
	cv::Point p(e->x(), e->y());
	const auto elapsed = std::chrono::system_clock::now() - _lastMouseEventTime;
	if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 1) {
		switch (_currentState) 
		{
		case State::SetTag:
			// _orient[1] = cv::Point(e->x(), e->y());
			break;
		case State::SetP0:
			_activeGrid->setCenter(p);
			break;
		case State::SetP1:
		{
			_activeGrid->zRotateTowardsPointInPlane(p);
			break;
		}		
		case State::SetP2:
			// setTheta(cv::Point(e->x(), e->y()));
			break;
		default:
			return;
		}
		emit update();
		_lastMouseEventTime = std::chrono::system_clock::now();
	}
}

//check if MOUSE BUTTON IS RELEASED
void BeesBookTagMatcher::mouseReleaseEvent(QMouseEvent * e) 
{
	//if (_currentState == State::SetBit)
		_currentState = State::SetP0;

	//// left button released
	//if (e->button() == Qt::LeftButton) {
	//	switch (_currentState) {
	//	//center and orientation of the tag were set.
	//	case State::SetTag:
	//	{
	//		// update active frame number and active grid
	//		_activeFrameNumber = _currentFrameNumber;

	//		const size_t newID = _trackedObjects.empty() ? 0 : _trackedObjects.back().getId() + 1;

	//		// insert new trackedObject object into _trackedObjects ( check if empty "first")
	//		_trackedObjects.emplace_back(newID);

	//		// associate new (active) grid to frame number
	//		_activeGrid = std::make_shared<Grid>(_orient[0], getAlpha(), newID);
	//		_trackedObjects.back().add(_currentFrameNumber, _activeGrid);

	//		//length of the vector is taken into consideration
	//		setTheta(cv::Point(e->x(), e->y()));
	//		_currentState = State::Ready;

	//		setNumTags();
	//		break;
	//	}
	//	//the tag was translated
	//	case State::SetP0:
	//		_currentState = State::Ready;
	//		/*_activeGrid->updateVectors();*/
	//		break;
	//	//orientation of the bee and the marker were modified
	//	case State::SetP1:
	//		_currentState = State::Ready;
	//		_setOnlyOrient = false;
	//		/*_activeGrid->updateVectors();*/
	//		break;
	//	//orientation of the marker was modified
	//	case State::SetP2:
	//		_currentState = State::Ready;
	//		/*_activeGrid->updateVectors();*/
	//		break;
	//	default:
	//		break;
	//	}
	//	emit update();
	//}
}

void BeesBookTagMatcher::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Plus || e->key() == Qt::Key_Minus) {
		if (_activeGrid) {
			const float direction = e->key() == Qt::Key_Plus ? 1.f : -1.f;
			//scale variable is updated by 0.05
			/*_activeGrid->scale = _activeGrid->scale + direction * 0.05;
			_activeGrid->updateAxes();*/
			emit update();
		}
	} else if (e->key() == Qt::Key_C && e->modifiers().testFlag(Qt::ControlModifier)) {
		_idCopyBuffer.clear();
		for (const TrackedObject& object : _trackedObjects) {
			// store ids of all grids on current frame in copy buffer
			if (object.count(_currentFrameNumber)) {
				_idCopyBuffer.insert(object.getId());
			}
		}
		_copyFromFrame = _currentFrameNumber;
	} else if (e->key() == Qt::Key_V && e->modifiers().testFlag(Qt::ControlModifier) && _copyFromFrame) {
		for (TrackedObject& object : _trackedObjects) {
			// check if id of object is in copy buffer
			if (_idCopyBuffer.count(object.getId())) {
				// check if grid from copy-from framenumber still exists
				const auto maybeGrid = object.maybeGet<Grid>(_copyFromFrame.get());
				// and create a copy if a grid with the same id does not
				// already exist on the current frame
				if (maybeGrid && !object.maybeGet<Grid>(_currentFrameNumber)) {
					object.add(_currentFrameNumber, std::make_shared<Grid>(*maybeGrid));
				}
			}
		}
		emit update();
		setNumTags();
	}
	else {
		switch (e->key())
		{
		case Qt::Key_H:
			_activeGrid->setYRotation(_activeGrid->getYRotation() + 0.05);
			break;
		case Qt::Key_G:
			_activeGrid->setYRotation(_activeGrid->getYRotation() - 0.05);
			break;
		case Qt::Key_W:
			_activeGrid->setXRotation(_activeGrid->getXRotation() - 0.05);
			break;
		case Qt::Key_S:
			_activeGrid->setXRotation(_activeGrid->getXRotation() + 0.05);
			break;
		case Qt::Key_A:
			_activeGrid->setZRotation(_activeGrid->getZRotation() - 0.05);
			break;
		case Qt::Key_D:
			_activeGrid->setZRotation(_activeGrid->getZRotation() + 0.05);
			break;
		default:
			return;
		}
		emit update();
	}
}

//BeesBookTagMatcher private member functions

//DRAWING FUNCTIONS

//function that draws the set Tags so far.
void BeesBookTagMatcher::drawSetTags(cv::Mat& image) const
{
	//// iterate over all stored object
	//for ( const TrackedObject& trackedObject : _trackedObjects ) 
	//{
	//	// check: data for that frame exists
	//	if ( trackedObject.count( _currentFrameNumber ) ) 
	//	{
	//		// get grid
	//		std::shared_ptr<Grid> grid = trackedObject.get<Grid>(_currentFrameNumber);
	//		
	//		// do not paint grid if it is active (will be drawn with different color later)
	//		if (grid != _activeGrid)
	//		{
	//			grid->drawFullTag(image, 2);
	//		}
	//	}
	//}
}

//function that draws the orientation vector while being set.
void BeesBookTagMatcher::drawOrientation(cv::Mat &image, const std::vector<cv::Point>& orient) const
{
	cv::line(image, orient[0], orient[1], cv::Scalar(0, 0, 255), 1);        //the orientation vector is printed in red
}

//function that draws an active tag calling an instance of Grid
void BeesBookTagMatcher::drawActiveTag(cv::Mat &image) const
{
	_activeGrid->draw(image,1);         //the grid is drawn as active
	//_activeGrid->updatePoints();
	//for (int i = 0; i < 3; i++)
	//{
	//	cv::circle(image, _activeGrid->absPoints[i], 1, COLOR_RED, 1);                 //the point is drawn in red
	//}
	////active point in blue

	//switch (_currentState) {
	//case State::SetP0:
	//	cv::circle(image, _activeGrid->absPoints[0], 1, COLOR_BLUE, 1);
	//	break;
	//case State::SetP1:
	//	cv::circle(image, _activeGrid->absPoints[1], 1, COLOR_BLUE, 1);
	//	break;
	//case State::SetP2:
	//	cv::circle(image, _activeGrid->absPoints[2], 1, COLOR_BLUE, 1);
	//	break;
	//default:
	//	break;
	//}
}

//function that draws the tag while being rotated in space
void BeesBookTagMatcher::drawSettingTheta(cv::Mat &img) const
{
	//cv::ellipse(img, _activeGrid->absPoints[0], _activeGrid->axesTag, _activeGrid->angleTag, 0, 360, COLOR_YELLOW, 1);
	//cv::line(img, _activeGrid->absPoints[0], _activeGrid->absPoints[0] + _activeGrid->realCoord[1], COLOR_RED, 1);          //the orientation vector is printed in red
	//cv::line(img, _activeGrid->absPoints[0], _activeGrid->absPoints[0] + _activeGrid->realCoord[2], COLOR_GREEN, 1);        //the redius vector is printed in green

	//for (int i = 0; i < 3; i++)
	//{
	//	cv::circle(img, _activeGrid->absPoints[0] + _activeGrid->realCoord[i], 1, COLOR_RED, 1);               //the point is drawn in red
	//}
	//switch (_currentState) {
	//case State::SetP1:
	//	cv::circle(img, _activeGrid->absPoints[0] + _activeGrid->realCoord[1], 1, COLOR_BLUE, 1);
	//	break;
	//case State::SetP2:
	//	cv::circle(img, _activeGrid->absPoints[0] + _activeGrid->realCoord[2], 1, COLOR_BLUE, 1);
	//	break;
	//default:
	//	break;
	//}
}

//TAG CONFIGURATION FUNCTIONS

//function called while setting the tag (it initializes orient vector)
void BeesBookTagMatcher::setTag(const cv::Point& location) 
{
	_currentState = State::SetTag;

	// empty vector
	_orient.clear();

	// first point of line
	_orient.push_back(location);

	// second point of line (will be updated when mouse is moved)
	_orient.push_back(location); 
	
	//the orientation vector is drawn.
	emit update();          
}

//function that allows P1 and P2 to be modified to calculate the tag's angle in space.
void BeesBookTagMatcher::setTheta(const cv::Point &location) 
{
	//double prop;
	//double angle;

	//switch (_currentState) {
	//case State::SetP1:
	//	//the length of the vector is equal to the distance to the pointer, limited to axisTag and 0.5*axisTag
	//	prop = dist(_activeGrid->absPoints[0], location) / (_activeGrid->scale * BeesBookTag::AXISTAG);
	//	if (prop > 1)
	//		prop = 1;
	//	else if (prop < 0.5)
	//		prop = 0.5;
	//	//P1 updates the orientation of the tag too.
	//	_activeGrid->orientation(location);
	//	_activeGrid->realCoord[1] = _activeGrid->polar2rect(prop * BeesBookTag::AXISTAG, _activeGrid->alpha);
	//	break;
	//case State::SetP2:
	//	//the length of the vector is equal to the distance to the pointer, limited to axisTag and 0.5*axisTag
	//	prop = dist(_activeGrid->absPoints[0], location) / (_activeGrid->scale * BeesBookTag::AXISTAG);
	//	if (prop > 1)
	//		prop = 1;
	//	else if (prop < 0.5)
	//		prop = 0.5;
	//	//P2 doesn't update alpha
	//	angle = atan2(location.x - _activeGrid->centerGrid.x, location.y - _activeGrid->centerGrid.y) - CV_PI / 2;
	//	_activeGrid->realCoord[2] = _activeGrid->polar2rect(prop * BeesBookTag::AXISTAG, angle);
	//	break;
	//default:
	//	break;
	//}
	////updates parameters
	//_activeGrid->updateParam();
	return;
}

//function that checks if one of the set Points is selected, returns true when one of the points is selected
bool BeesBookTagMatcher::selectPoint(const cv::Point& location) {
	//for (int i = 0; i < 3; i++) {
	//	if (dist(location, _activeGrid->absPoints[i]) < 2) //check if the pointer is on one of the points
	//	{
	//		switch (i)
	//		{
	//		case 0:
	//			_currentState = State::SetP0;
	//			return true;
	//		case 1:
	//			_currentState = State::SetP1;
	//			return true;
	//		case 2:
	//			_currentState = State::SetP2;
	//			return true;
	//		}
	//	}
	//}
	return false;
}

//function that checks if one of the already set Tags is selected.
void BeesBookTagMatcher::selectTag(const cv::Point& location)
{
	//// iterate over all stored objects
	//for (size_t i = 0; i < _trackedObjects.size(); i++) 
	//{
	//	// get pointer to i-th object
	//	std::shared_ptr<Grid> grid = _trackedObjects[i].maybeGet<Grid>(_currentFrameNumber);

	//	// check if grid is valid
	//	if (grid && dist(location, grid->centerGrid) < grid->axesTag.height) 
	//	{
	//		// assign the found grid to the activegrid pointer
	//		_activeGrid        = grid;
	//		_activeFrameNumber = _currentFrameNumber;

	//		emit update();

	//		return;
	//	}
	//}
}

void BeesBookTagMatcher::cancelTag()
{
	_activeGrid.reset();
	_activeFrameNumber.reset();
	_currentState  = State::Ready;
	_setOnlyOrient = false;
}

void BeesBookTagMatcher::removeCurrentActiveTag()
{	
	//if (_activeGrid)
	//{
	//	auto trackedObjectIterator = std::find_if(_trackedObjects.begin(), _trackedObjects.end(),
	//	                                          [&](const TrackedObject & o){ return o.getId() == _activeGrid->objectId; }) ;
	//	
	//	assert( trackedObjectIterator != _trackedObjects.end() );

	//	trackedObjectIterator->erase(_currentFrameNumber);

	//	// if map empty
	//	if (trackedObjectIterator->isEmpty())
	//	{
	//		// delete from _trackedObjects
	//		_trackedObjects.erase(trackedObjectIterator);
	//	}
	//
	//	// reset active tag and frame and...
	//	cancelTag();

	//	setNumTags();
	//}
}

//AUXILIAR FUNCTION

//function that calculates the distance between two points
double BeesBookTagMatcher::dist(const cv::Point &p1, const cv::Point &p2) const
{
	const cv::Point diff = p1 - p2;
	return sqrt(diff.x * diff.x + diff.y * diff.y);
}

double BeesBookTagMatcher::getAlpha() const
{
	return atan2(_orient[1].x - _orient[0].x, _orient[1].y - _orient[0].y) - CV_PI / 2;
}

void BeesBookTagMatcher::setNumTags()
{
	size_t cnt = 0;
	for (size_t i = 0; i < _trackedObjects.size(); i++)
	{
		if (_trackedObjects[i].maybeGet<Grid>(_currentFrameNumber)) {
			++cnt;
		}
	}

	_UiToolWidget.numTags->setText(QString::number(cnt));
}

const std::set<Qt::Key> &BeesBookTagMatcher::grabbedKeys() const
{
	static const std::set<Qt::Key> keys { Qt::Key_Plus, Qt::Key_Minus,
	                                      Qt::Key_C, Qt::Key_V,
										  Qt::Key_W, Qt::Key_A,
										  Qt::Key_S, Qt::Key_D,
										  Qt::Key_G, Qt::Key_H };
	return keys;
}

bool BeesBookTagMatcher::event(QEvent *event)
{
	const QEvent::Type etype = event->type();
	switch (etype) {
	case QEvent::KeyPress:
		keyPressEvent(static_cast<QKeyEvent*>(event));
		return true;
		break;
	case QEvent::MouseButtonPress:
		mousePressEvent(static_cast<QMouseEvent*>(event));
		return true;
		break;
	case QEvent::MouseButtonRelease:
		mouseReleaseEvent(static_cast<QMouseEvent*>(event));
		return true;
		break;
	case QEvent::MouseMove:
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
		return true;
		break;
	default:
		event->ignore();
		return false;
	}
}
