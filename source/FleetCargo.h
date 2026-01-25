/* FleetCargo.h
Copyright (c) 2022 by warp-core

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

#include "Shop.h"

class DataNode;
class Outfit;
class Ship;



// A collection of cargo settings to be applied to ships from a Fleet or NPC.
class FleetCargo {
public:
	void Load(const DataNode &node);
	void LoadSingle(const DataNode &node);

	void SetCargo(Ship *ship) const;


private:
	// The number of different items this object can assign to ships.
	int cargo = 3;
	std::vector<std::string> commodities;
	std::set<const Shop<Outfit> *> outfitters;
};
