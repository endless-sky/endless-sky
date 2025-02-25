/* Depreciation.h
Copyright (c) 2016 by Michael Zahniser

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
#include <memory>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Outfit;
class Ship;



// Class for tracking depreciation records, by storing the day on which a given
// outfit or ship was purchased. Any ship or outfit for which no record exists,
// for example because it is plunder, counts as full depreciated.
class Depreciation {
public:
	// What fraction of its cost a fully depreciated item has left:
	static double Full();


public:
	// Load or save depreciation records. Multiple records may be saved in the
	// player info, under different names (e.g. fleet and stock depreciation).
	void Load(const DataNode &node);
	void Save(DataWriter &out, int day) const;
	// Check if any records have been loaded.
	bool IsLoaded() const;
	// If no records have been loaded, initialize with an entire fleet.
	void Init(const std::vector<std::shared_ptr<Ship>> &fleet, int day);

	// Add a ship, and all its outfits, to the depreciation record.
	void Buy(const Ship &ship, int day, Depreciation *source = nullptr, bool chassisOnly = false);
	// Add a single outfit to the depreciation record.
	void Buy(const Outfit *outfit, int day, Depreciation *source = nullptr);

	// Get the value of an entire fleet.
	int64_t Value(const std::vector<std::shared_ptr<Ship>> &fleet, int day, bool chassisOnly = false) const;
	// Get the value of a ship, along with all its outfits.
	int64_t Value(const Ship &ship, int day) const;
	// Get the value just of the chassis of a ship.
	int64_t Value(const Ship *ship, int day, int count = 1) const;
	// Get the value of an outfit.
	int64_t Value(const Outfit *outfit, int day, int count = 1) const;


private:
	// "Sell" an item, removing it from the given record and returning the base
	// day for its depreciation.
	int Sell(std::map<int, int> &record) const;
	// Calculate depreciation:
	double Depreciate(const std::map<int, int> &record, int day, int count = 1) const;
	double Depreciate(int age) const;
	// Depreciation of an item for which no record exists. If buying, items
	// default to no depreciation. When selling, they default to full.
	double DefaultDepreciation() const;


private:
	// This depreciation record is either a planet's stock or a player's fleet.
	// If it's the stock, it sells you the most depreciated item first, and once
	// it runs out of depreciated items all the rest have full price. If it is
	// your fleet, you sell the least depreciated items first.
	bool isStock = true;
	// Check if any data has been loaded.
	bool isLoaded = false;

	std::map<const Ship *, std::map<int, int>> ships;
	std::map<const Outfit *, std::map<int, int>> outfits;
};
