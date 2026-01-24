/* ShipManager.h
Copyright (c) 2022 by Hurleveur

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

#include <map>
#include <memory>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class PlayerInfo;
class Ship;



// Used to contain and manage gift/take ship, and owns commands.
class ShipManager {
public:
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Returns if the player meets the conditions; if they have the ships ready to be taken.
	bool CanBeDone(const PlayerInfo &player) const;
	// Give or take the ships.
	void Do(PlayerInfo &player) const;
	// Expands phrases and substitutions in the ship name, into a new copy of this ShipManager
	ShipManager Instantiate(const std::map<std::string, std::string> &subs) const;
	// The model of the concerned ship.
	const Ship *ShipModel() const;
	// The identifier that the given/taken ship will have.
	const std::string &Id() const;
	bool Giving() const;


private:
	// Get a list of ships that satisfies these conditions, to take them away later.
	std::vector<std::shared_ptr<Ship>> SatisfyingShips(const PlayerInfo &player) const;


private:
	const Ship *model = nullptr;
	std::string name;
	std::string id;
	int count = 1;
	bool taking = false;
	bool unconstrained = false;
	bool requireOutfits = false;
	bool takeOutfits = false;
};
