/* Variant.h
Copyright (c) 2019 by Jonathan Steck

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
	
	// Choose a list of ships from this variant. All ships from the ships
	// vector are chosen, as well as a random selection of ships from any
	// nested variants in the stockVariants or variants vectors.
	std::vector<const Ship *> ChooseShips() const;
	// Choose a ship from this variant given that it is a nested variant.
	// Nested variants only choose a single ship from among their list
	// of ships and variants.
	const Ship *NestedChooseShip() const;
	
	// The average credit worth of this variant.
	int64_t Strength() const;
	// The average credit worth of this variant if it is nested within
	// another variant.
	int64_t NestedStrength() const;
	
	// A static function used by Variant and Fleet to randomly choose a single
	// variant between a list of normal variants and a list of stock variants,
	// given the total weight between the two and the total weight of the stock
	// variants.
	static const Variant &ChooseVariant(const std::vector<std::pair<Variant, int>> &nVariants, const std::vector<std::pair<const Variant *, int>> &sVariants, int vTotal, int sTotal);
	
	bool operator==(const Variant &other) const;

private:
	bool NestedInSelf(std::string check) const;
	
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
