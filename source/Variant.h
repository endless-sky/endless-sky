/* Variant.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef VARIANT_H_
#define VARIANT_H_

#include "WeightedUnionItem.h"

#include <cstdint>
#include <string>
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

	void Load(const DataNode &node);

	// Determine if this variant template uses well-defined data.
	bool IsValid() const;

	// Choose a list of ships from this variant. All ships from the ships
	// vector are chosen, as well as a random selection of ships from any
	// nested variants.
	std::vector<const Ship *> ChooseShips() const;

	// The strength of a variant is the sum of the cost of its ships and
	// the strength of any nested variants.
	int64_t Strength() const;

	bool operator==(const Variant &other) const;
	bool operator!=(const Variant &other) const;


private:
	// Check whether a variant is contained within itself.
	bool NestedInSelf(const std::string &check) const;
	// Determine if this variant template uses well-defined data as a
	// nested variant.
	bool NestedIsValid() const;
	// Choose a ship from this variant given that it is a nested variant.
	// Nested variants only choose a single ship from among their list
	// of ships and variants.
	const Ship *NestedChooseShip() const;
	// The strength of a nested variant is its normal strength divided by
	// the total weight of its contents.
	int64_t NestedStrength() const;


private:
	std::string name;
	std::vector<const Ship *> ships;
	std::vector<WeightedUnionItem<Variant>> variants;
};



#endif
