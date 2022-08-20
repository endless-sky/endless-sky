/* ShipManager.h
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIP_MANAGER_H_
#define SHIP_MANAGER_H_

#include "DataNode.h"
#include "Ship.h"

#include <string>

class PlayerInfo;



// Used to contain and manage gift/take ship, and owns commands.
class ShipManager {
public:
	ShipManager() = default;
	ShipManager(std::string name, int count = 1, bool unconstrained = false, bool withOutfits = false);

	std::vector<std::shared_ptr<Ship>> SatisfyingShips(const PlayerInfo &player, const Ship *model) const;
	bool Satisfies(const PlayerInfo &player, const Ship *model) const;

	const std::string &Name() const;
	int Count() const;
	bool Unconstrained() const;
	bool WithOutfits() const;


public:
	// Will try to add the a shipManager matching the DataNode to the given shipList.
	static void Load(const DataNode &child, std::map<const Ship *, ShipManager> shipsList);


private:
	std::string name;
	int count;
	bool unconstrained;
	bool withOutfits;
};



#endif
