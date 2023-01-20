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

#ifndef SHIP_MANAGER_H_
#define SHIP_MANAGER_H_

#include <string>
#include <memory>
#include <vector>
#include <map>

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
	void Do(PlayerInfo &player) const;

	// The model of the concerned ship.
	const Ship *ShipModel() const;
	// The in game name of the given/taken ship.
	const std::string &Name() const;
	// The identifier that the given/taken ship will have.
	const std::string &Id() const;
	// The number of ships we will take/give.
	int Count() const;
	// If true, the ship will be taken no matter what; even if it is not in the same system, or parked.
	bool Unconstrained() const;
	// If true, the ship's outfits will be taken from the player, otherwise they will be left in the stock.
	bool WithOutfits() const;


private:
	// Get a list of ships that satisfies these conditions, to take them away later.
	std::vector<std::shared_ptr<Ship>> SatisfyingShips(const PlayerInfo &player) const;


private:
	const Ship *model = nullptr;
	std::string name;
	std::string id;
	int count = 1;
	bool unconstrained = false;
	bool withOutfits = false;
};



#endif
