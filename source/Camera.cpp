/* Camera.cpp
Copyright (c) 2023 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Camera.h"

#include "Angle.h"
#include "Point.h"
#include "Preferences.h"

#include <vector>

using namespace std;

namespace {
	static const double CAMERA_SMOOTHNESS = 0.05;
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

	bool enabled = true;
}

void Camera::Enable(bool newEnabled)
{
	enabled = newEnabled;
}

Point Camera::Offset()
{
	return enabled ? finalCameraPosition - center : Point();
}

Point Camera::VelocityOffset()
{
	return enabled ? cameraVelocity - centerVelocity : Point();
}

Point Camera::Position()
{
	return enabled ? finalCameraPosition : center;
}

Point Camera::Velocity()
{
	return enabled ? cameraVelocity : centerVelocity;
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
	case State::JUMPING:
		cameraVelocity += (flagshipVelocity - cameraVelocity) * 0.1;
		cameraCenter += cameraVelocity;
		cameraCenter += (flagshipCenter - cameraCenter) * 0.05;
		break;
	case State::JUMPED:
		cameraCenter += (flagshipCenter - cameraCenter) * 0.006;
		break;
	case State::WORMHOLED:
		cameraCenter += (flagshipCenter - cameraCenter) * 0.005;
		break;
	}
	if(Preferences::GetCameraSetting() == Preferences::DynamicCamera::FORWARD)
		finalCameraPosition = center - (cameraCenter - center);
	else
		finalCameraPosition = cameraCenter;
	finalCameraPosition = finalCameraPosition + (targetPoint) * 0.4;
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
	return enabled ? zoom : 1.;
}

void Camera::SetZoom(double newZoom)
{
	if(state == State::JUMPED)
		trueZoom = newZoom * .6;
	else
		trueZoom = newZoom;
}

void Camera::SetAbsoluteZoom(double newZoom)
{
	if(state == State::WORMHOLED)
		zoom = newZoom * .75;
	else
		zoom = newZoom;
}

void Camera::SetTarget(Point newTargetPos)
{
	targetPoint = newTargetPos;
}
