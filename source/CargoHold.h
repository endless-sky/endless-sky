/* CargoHold.h
Copyright (c) 2014 by Michael Zahniser

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

#include <cstdint>
#include <map>
#include <string>

class Conversation;
class DataNode;
class DataWriter;
class Government;
class Mission;
class Outfit;
class PlayerInfo;
class System;



// This class represents the cargo and passengers that a ship can carry. That
// can include ordinary commodities, plundered outfits, and mission cargo. When
// you land on a planet, all your cargo is pooled into a single collection, not
// tied to any one ship, so you retain it even if you sell off all your ships.
// When you take off, cargo is distributed among your ships, and if some of it
// will not fit it must be sold off.
class CargoHold {
public:
	void Clear();

	// Load the cargo manifest from a DataFile. This must be done after the
	// GameData is loaded, so that the sizes of any outfits are known.
	void Load(const DataNode &node);
	// Save the cargo manifest to a file.
	void Save(DataWriter &out) const;

	// Set the capacity of this cargo hold.
	void SetSize(int tons);
	int Size() const;
	int Free() const;
	double FreePrecise() const;
	int Used() const;
	double UsedPrecise() const;
	int CommoditiesSize() const;
	int OutfitsSize() const;
	double OutfitsSizePrecise() const;
	bool HasOutfits() const;
	int MissionCargoSize() const;
	bool HasMissionCargo() const;
	bool IsEmpty() const;

	// Set the number of free bunks for passengers.
	void SetBunks(int count);
	int BunksFree() const;
	int Passengers() const;

	// Normal cargo:
	int Get(const std::string &commodity) const;
	// Spare outfits:
	int Get(const Outfit *outfit) const;
	// Mission cargo:
	int Get(const Mission *mission) const;
	int GetPassengers(const Mission *mission) const;

	const std::map<std::string, int> &Commodities() const;
	const std::map<const Outfit *, int> &Outfits() const;
	// Note: some missions may have cargo that takes up 0 space, but should
	// still show up on the cargo listing.
	const std::map<const Mission *, int> &MissionCargo() const;
	const std::map<const Mission *, int> &PassengerList() const;

	// For all the transfer functions, the "other" can be null if you simply want
	// the commodity to "disappear" or, if the "amount" is negative, to have an
	// unlimited supply. The return value is the actual number transferred.
	int Transfer(const std::string &commodity, int amount, CargoHold &to);
	int Transfer(const Outfit *outfit, int amount, CargoHold &to);
	int Transfer(const Mission *mission, int amount, CargoHold &to);
	int TransferPassengers(const Mission *mission, int amount, CargoHold &to);
	// Transfer as much as the given cargo hold has capacity for. The priority is
	// first mission cargo, then spare outfits, then ordinary commodities.
	void TransferAll(CargoHold &to, bool transferPassengers = true);

	// These functions do the same thing as Transfer() with no destination
	// specified, but they have clearer names to make the code more readable.
	int Add(const std::string &commodity, int amount = 1);
	int Add(const Outfit *outfit, int amount = 1);
	int Remove(const std::string &commodity, int amount = 1);
	int Remove(const Outfit *outfit, int amount = 1);

	// Add or remove any cargo or passengers associated with the given mission.
	void AddMissionCargo(const Mission *mission);
	void RemoveMissionCargo(const Mission *mission);

	// Get the total value of all this cargo, in the given system.
	int64_t Value(const System *system) const;

	// If anything you are carrying is illegal, return the maximum fine you can
	// be charged for any illegal outfits plus the sum of the fines for all
	// missions. If the returned value is negative, you are carrying something
	// or someone that warrants a death sentence for you.
	std::pair<int, const Conversation *> IllegalCargoFine(const Government *government) const;
	int IllegalPassengersFine(const Government *government) const;

	// Returns the amount tons of illegal cargo.
	int IllegalCargoAmount() const;


private:
	// Use -1 to indicate unlimited capacity.
	int size = -1;
	int bunks = -1;

	// Track how many objects of each type are being carried:
	std::map<std::string, int> commodities;
	std::map<const Outfit *, int> outfits;
	std::map<const Mission *, int> missionCargo;
	std::map<const Mission *, int> passengers;
};
