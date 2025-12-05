/* FreeCamera.h
Copyright (c) 2024 by the Endless Sky developers

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

#include "CameraController.h"
#include "Point.h"

#include <string>

// Free-roaming camera controlled by keyboard.
class FreeCamera : public CameraController {
public:
	FreeCamera();

	Point GetTarget() const override;
	Point GetVelocity() const override;
	void Step() override;
	const std::string &ModeName() const override;

	// Set movement direction from input (-1 to 1 for each axis)
	void SetMovement(double x, double y);

	// Set position directly (e.g., when switching to this mode)
	void SetPosition(const Point &pos);


private:
	Point position;
	Point velocity;
	Point inputDirection;
	double speed = 3.;      // Slower, more controllable
	double friction = 0.92; // More friction for smoother stops
	static const std::string name;
};
