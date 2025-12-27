/* Camera.h
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

#pragma once

#include "Point.h"



// A class that represents the viewport while in space.
class Camera {
public:
	// Instantly snap the camera to the given target.
	void SnapTo(const Point &target, bool keepVelocity = false);
	// Move the camera toward the given target.
	void MoveTo(const Point &target, double hyperspaceInfluence);

	// The position of the camera's center.
	const Point &Center() const;
	// The velocity that the camera is moving at. Used for motion blur and starfield movement.
	const Point &Velocity() const;


private:
	Point center;
	Point velocity;

	Point accel;
	Point oldTarget;
};
