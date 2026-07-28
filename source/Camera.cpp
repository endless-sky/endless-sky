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

#include "Preferences.h"

#include <cmath>

using namespace std;

namespace {
	const double CAMERA_VELOCITY_TRACKING = 0.1;
	const double CAMERA_POSITION_CENTERING = 0.01;
}



void Camera::SnapTo(const Point &target, bool keepVelocity)
{
	// If a player is jumping to a new system, preserve their velocity by offsetting oldTarget
	// and center by the current velocity.
	Point newTarget = keepVelocity ? target - velocity : target;
	oldTarget = newTarget;
	center = newTarget;
	// Velocity is zeroed here even when keepVelocity is true, but it will be re-calculated
	// when MoveTo is called later in this frame.
	velocity = Point();
	accel = Point();
}



void Camera::MoveTo(const Point &target, double hyperspaceInfluence)
{
	Point baseVelocity = target - oldTarget;
	oldTarget = target;
	if(Preferences::CameraAcceleration() == Preferences::CameraAccel::OFF)
	{
		center = target;
		velocity = baseVelocity;
		accel = Point();
		return;
	}

	double accelMultiplier = Preferences::CameraAcceleration() == Preferences::CameraAccel::REVERSED ? -1. : 1.;

	// Flip the accel offset if accelMultiplier is negative to simplify logic.
	const Point oldAbsAccel = baseVelocity.Lerp(accel, accelMultiplier);
	const Point newAbsAccel = oldAbsAccel.Lerp(baseVelocity, CAMERA_VELOCITY_TRACKING);
	Point newCenter = (center + newAbsAccel).Lerp(target, CAMERA_POSITION_CENTERING);

	// Increase the interpolation speed when traveling through hyperspace so that the camera doesn't lose focus
	// on the target.
	if(hyperspaceInfluence)
		newCenter = newCenter.Lerp(target, pow(hyperspaceInfluence, .5));

	velocity = newCenter - center;
	center = newCenter;
	// Acceleration interpolation is disabled while in hyperspace.
	// Flip the accel back over the baseVelocity when not in hyperspace.
	accel = hyperspaceInfluence ? baseVelocity : baseVelocity.Lerp(newAbsAccel, accelMultiplier);
}



const Point &Camera::Center() const
{
	return center;
}



const Point &Camera::Velocity() const
{
	return velocity;
}
