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

#include "Point.h"
#include "Weapon.h"

#include <list>
#include <memory>

class DataNode;
class Ship;



// Controls for how an NPC fleet should be placed into a system.
class FleetPlacement {
public:
	FleetPlacement() = default;
	void Load(const DataNode &node);

	void Place(const std::list<std::shared_ptr<Ship>> &ships, bool isEntering) const;


private:
	bool loaded = false;
	// If this NPC spawns already present in the system, place it this far from the system center at a random angle.
	bool setDistance = false;
	double distance = 0.;
	// If this NPC spawns already present in the system, place it at this exact position in the system.
	bool setPosition = false;
	Point position;
	// If this NPC contains multiple ships, this is the distance for how far spread out the ships should be from
	// one another relative to the placement location.
	double spread = 500.;
	// A weapon whose damage is to be applied to ships when placed.
	Weapon weapon;
};
