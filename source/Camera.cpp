#include "Camera.h"
#include "Camera.h"
#include "Camera.h"

using namespace std;

namespace {
	static const double CAMERA_SMOOTHNESS = 0.025;
	static const double ZOOM_BLEND = 0.032;

	Point center = Point();
	Point centerVelocity = Point();

	Point cameraCenter = Point();
	Point cameraVelocity = Point();

	Point finalCameraPosition = Point();

	Point targetPoint = Point();

	double zoom = 1.;
	double trueZoom = 1.;

	Camera::State state = Camera::State::NORMAL;

	vector<pair<Angle, double>> cameraShake = {};
}

Point Camera::Offset()
{
	return finalCameraPosition - center;
}

Point Camera::VelocityOffset()
{
	return cameraVelocity - centerVelocity;
}

Point Camera::Position()
{
	return finalCameraPosition;
}

Point Camera::Velocity()
{
	return cameraVelocity;
}

Point Camera::CenterPos()
{
	return center;
}

Point Camera::CenterVel()
{
	return centerVelocity;
}

void Camera::SetPosition(Point newPosition)
{
	cameraCenter = newPosition;
}

void Camera::SetVelocity(Point newVelocity)
{
	cameraVelocity = newVelocity;
}

void Camera::SetOffset(Point newOffset)
{
	cameraCenter = center + newOffset;
}

void Camera::SetVelocityOffset(Point newVelocity)
{
	cameraVelocity = centerVelocity + newVelocity;
}

void Camera::Update(Point flagshipCenter, Point flagshipVelocity)
{
	zoom += (trueZoom - zoom) * ZOOM_BLEND;
	switch(state)
	{
	case State::NORMAL:
		cameraVelocity += (flagshipVelocity - cameraVelocity) * CAMERA_SMOOTHNESS;
		cameraCenter += cameraVelocity;
		cameraCenter += (flagshipCenter - cameraCenter) * CAMERA_SMOOTHNESS;
		break;
	case State::HYPERJUMPING:
		cameraVelocity += (flagshipVelocity - cameraVelocity) * 0.1;
		cameraCenter += cameraVelocity;
		cameraCenter += (flagshipCenter - cameraCenter) * 0.01;
		break;
	case State::HYPERJUMPED:
		cameraVelocity = flagshipVelocity;
		cameraCenter += cameraVelocity;
		cameraCenter += (flagshipCenter - cameraCenter) * 0.1;
		break;
	case State::JUMPED:
		cameraCenter += (flagshipCenter - cameraCenter) * 0.01;
		break;
	case State::WORMHOLED:
		cameraCenter += (flagshipCenter - cameraCenter) * 0.005;
		break;
	}
	finalCameraPosition = cameraCenter + (targetPoint) * 0.4;
}

void Camera::SetCenter(Point newCenter, Point newVelocity)
{
	center = newCenter;
	centerVelocity = newVelocity;
}

void Camera::SetState(State newState)
{
	state = newState;
}

Camera::State Camera::GetState()
{
	return state;
}

double Camera::GetZoom()
{
	return zoom;
}

void Camera::SetZoom(double newZoom)
{
	if(state == State::WORMHOLED)
		trueZoom = newZoom * .5;
	else
		trueZoom = newZoom;
}

void Camera::SetTarget(Point newTargetPos)
{
	targetPoint = newTargetPos;
}
