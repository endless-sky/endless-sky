/* FleetPlacement.h
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

#include "Orbit.h"
#include "Point.h"
#include "Weapon.h"

#include <list>
#include <memory>
#include <optional>


class DataNode;
class Date;
class Ship;



// Controls for how an NPC fleet should be placed into a system.
class FleetPlacement {
public:
	FleetPlacement() = default;
	void Load(const DataNode &node);

	void Place(const std::list<std::shared_ptr<Ship>> &ships, const Date &date, bool isEntering) const;


private:
	bool loaded = false;
	// The distance from the system center to place the NPCs. The angle will be randomized if not set.
	std::optional<double> distance;
	std::optional<Angle> angle;
	// The orbit to place the NPCs on. The position in the orbit will match the position
	// of a SetllarObject with the same orbit.
	std::optional<Orbit> orbit;
	// The exact position to place the NPCs.
	std::optional<Point> position;
	// The velocity and facing angle to place the NPCs with.
	std::optional<Point> velocity;
	// If this NPC contains multiple ships, this is the distance for how far spread out the ships should be from
	// one another relative to the placement location. The first ship will be placed in the center, with all subsequent
	// ships choosing a random angle and a random distance away from the center up to this value.
	double spread = 500.;
	// A weapon whose damage is applied to ships when placed.
	Weapon weapon;
};
