/* LoadingCircle.h
Copyright (c) 2026 by Amazinite

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

#include "Angle.h"

class Point;



// A class for drawing loading circles.
class LoadingCircle {
public:
	LoadingCircle(float size, int ticks, double rotationSpeed = 0.);

	// Rotate the initial tick mark position, if this loading circle has a rotation speed.
	void Step();
	// Provide the position of the center of the circle and the progress percentage.
	void Draw(const Point &position, double progress = 1.) const;


private:
	// The size of the circle;
	float size;
	// The number of tick marks that should be displayed.
	int ticks;
	// The number of degrees that each tick is offset from the previous one.
	Angle angleOffset;
	// The amount of rotation to apply to the position of the starting tick in the circle every step.
	double rotationSpeed;
	// The current amount of rotation applied to the starting tick.
	double rotation;
};
