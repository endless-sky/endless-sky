/* Camera.cpp
Copyright (c) 2025 by Amazinite

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

#include "Logger.h"
#include "Preferences.h"

#include <cmath>

using namespace std;

namespace {
	const double CAMERA_VELOCITY_TRACKING = 0.1;
	const double CAMERA_POSITION_CENTERING = 0.01;
}



void Camera::SnapTo(const Point &target)
{
	oldTarget = target;
	center = target;
	velocity = Point();
}



void Camera::MoveTo(const Point &target, double influence, bool killVelocity)
{
	// The new base velocity is the change in position of the target between frames.
	Point baseVelocity = target - oldTarget;
	oldTarget = target;
	if(Preferences::CameraAcceleration() == Preferences::CameraAccel::OFF)
	{
		center = target;
		velocity = baseVelocity;
		return;
	}

	double cameraAccelMultiplier = Preferences::CameraAcceleration() == Preferences::CameraAccel::REVERSED ? -1. : 1.;

	// Flip the velocity offset if cameraAccelMultiplier is negative to simplify logic.
	const Point absoluteOldCenterVelocity = baseVelocity.Lerp(velocity, cameraAccelMultiplier);

	const Point newAbsVelocity = absoluteOldCenterVelocity.Lerp(baseVelocity, CAMERA_VELOCITY_TRACKING);

	Point newCenter = (center + newAbsVelocity).Lerp(target, CAMERA_POSITION_CENTERING);

	// Flip the velocity back over the baseVelocity.
	Point newVelocity = baseVelocity.Lerp(newAbsVelocity, cameraAccelMultiplier);

	center = newCenter.Lerp(target, pow(influence, .5));
	velocity = killVelocity ? baseVelocity : newVelocity.Lerp(baseVelocity, pow(influence, .5));
}



const Point &Camera::Center() const
{
	return center;
}



const Point &Camera::Velocity() const
{
	return velocity;
}
