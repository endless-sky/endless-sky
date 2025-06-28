/* Visual.h
Copyright (c) 2017 by Michael Zahniser

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

#include "Body.h"

#include "Angle.h"
#include "Point.h"

class Effect;



// A Visual is the object created by an Effect. This is a separate class from
// Effect to allow it to be much more lightweight.
class Visual : public Body {
public:
	Visual() = default;
	Visual(const Effect &effect, Point pos, Point vel, Angle facing, Point hitVelocity = Point(),
		double inheritedZoom = 1.);

	// Functions provided by the Body base class:
	// Frame GetFrame(int step = -1) const;
	// const Point &Position() const;
	// const Point &Velocity() const;
	// const Angle &Facing() const;
	// Point Unit() const;
	// double Zoom() const;

	// Step the effect forward.
	void Move();


private:
	Angle spin;
	int lifetime = 0;
};
