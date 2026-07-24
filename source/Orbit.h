/* Orbit.h
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

#include <utility>

class Angle;
class Point;


// A class representing the orbit of an object in a system.
class Orbit {
public:
	explicit Orbit(double distance = 0., double period = 0., double offset = 0.);

	// Given the current date as a number of days since the epoch,
	// calculate where an object with this orbit should be positioned.
	std::pair<Point, Angle> Position(double now) const;

	bool operator==(const Orbit &) const = default;


private:
	// Let System handle setting all the values of a StellarObject's orbit.
	friend class System;
	// Let StellarObject access its own orbit.
	friend class StellarObject;

	// The distance from the system center.
	double distance;
	// The orbital speed of the object. Divide by 360 to get the number of days it takes to orbit the center.
	double speed;
	// A number of degrees to offset the object by. This allows multiple objects to
	// share the same orbital distance while being at different locations.
	double offset;
};
