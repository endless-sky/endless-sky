#include "Camera.h"
#include "Camera.h"
#include "Camera.h"

using namespace std;

namespace {
	static const double CAMERA_SMOOTHNESS = 0.016;

	Point center = Point();
	Point centerVelocity = Point();

	Point cameraCenter = Point();
	 Point cameraVelocity = Point();

	double zoom = 0.;

	vector<pair<Angle, double>> cameraShake = {};
}

Point Camera::Offset()
{
	return cameraCenter - center;
}

Point Camera::VelocityOffset()
{
	return cameraVelocity - centerVelocity;
}

Point Camera::Position()
{
	return cameraCenter;
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
	cameraVelocity += (flagshipVelocity - cameraVelocity) * CAMERA_SMOOTHNESS;
	cameraCenter += cameraVelocity;
	cameraCenter += (flagshipCenter - cameraCenter) * CAMERA_SMOOTHNESS;
}

void Camera::SetCenter(Point newCenter, Point newVelocity)
{
	center = newCenter;
	centerVelocity = newVelocity;
}
