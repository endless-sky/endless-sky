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

#include "DataNode.h"

#include <optional>
#include <utility>

class Angle;
class DataNode;
class Point;


// A class representing the orbit of an object in a system.
class Orbit {
public:
	explicit Orbit(double distance = 0., double period = 0., double offset = 0.);
	explicit Orbit(const DataNode &node);

	void Load(const DataNode &node);

	double Distance() const;
	double Period() const;
	bool HasExplicitPeriod() const;
	double Offset() const;

	// Calculate the speed of this orbit given the mass of the object it is orbiting around.
	// If a distance is not given, uses the distance of this orbit.
	void CalculatePeriod(double mass, std::optional<double> dist = std::nullopt);
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
	double distance = 0.;
	// The orbital speed of the object, in degrees per day.
	double speed = 36.;
	// Whether an orbital period was explicitly set in game data. If false, this
	// orbit should calculate its period based off of the star(s) in the system.
	bool explicitPeriodSet = false;
	// A number of degrees to offset the object by. This allows multiple objects to
	// share the same orbital distance while being at different locations.
	double offset = 0.;
};
