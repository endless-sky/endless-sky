/* Camera.h
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

#include "Point.h"

#ifndef CAMERA_H

#define CAMERA_H

class Camera {
public:
	enum class State {
		NORMAL,
		HYPERJUMPING,
		HYPERJUMPED,
		JUMPING,
		JUMPED,
		WORMHOLED
	};

	static void Enable(bool newEnabled);

	static Point Offset();
	static Point VelocityOffset();

	static Point Position();
	static Point Velocity();

	static Point CenterPos();
	static Point CenterVel();

	static void SetPosition(Point newPosition);
	static void SetVelocity(Point newVelocity);
	static void SetOffset(Point newOffset);
	static void SetVelocityOffset(Point newVelocity);

	static void Update(Point flagshipCenter, Point flagshipVelocity);
	static void SetCenter(Point newCenter, Point newVelocity);

	static void SetState(State newState);
	static State GetState();

	static double GetZoom();
	static void SetZoom(double newZoom);
	static void SetAbsoluteZoom(double newZoom);

	static void SetTarget(Point newTargetPos);
};

#endif
