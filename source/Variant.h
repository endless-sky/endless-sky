/* Variant.h
Copyright (c) 2019 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef VARIANT_H_
#define VARIANT_H_

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class Ship;


// A variant represents a collection of ships that may be spawned by a fleet.
// Each variant contains one or more ships or variants.
class Variant {
public:
	Variant() = default;
	// Construct and Load() at the same time.
	Variant(const DataNode &node);
	
	void Load(const DataNode &node, bool global = false);
	
	const std::string &Name() const;
	std::vector<const Ship *> Ships() const;
	std::vector<std::pair<const Variant *, int>> StockVariants() const;
	std::vector<std::pair<Variant, int>> Variants() const;
	
	std::vector<const Ship *> ChooseShips() const;
	int64_t Strength() const;
	
	bool operator==(const Variant &other) const;
	
private:
	std::vector<const Ship *> NestedChooseShips() const;
	int64_t NestedStrength() const;
	
private:
	std::string name;
	int total = 0;
	int variantTotal = 0;
	int stockTotal = 0;
	std::vector<const Ship *> ships;
	// The stockVariants vector contains the variants that are defined as root
	// nodes and stored in GameData, while the variants vector contains those
	// variants defined by this fleet definition.
	std::vector<std::pair<const Variant *, int>> stockVariants;
	std::vector<std::pair<Variant, int>> variants;
};



#endif
