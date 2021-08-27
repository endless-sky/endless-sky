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

#include "WeightedList.h"

#include <string>
#include <vector>

class DataNode;
class Ship;
class WeightedVariant;


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
	// nested variants in the stockVariants or variants vectors.
	std::vector<const Ship *> ChooseShips() const;
	
	// The average credit worth of this variant.
	int64_t Strength() const;
	
	bool operator==(const Variant &other) const;
	bool operator!=(const Variant &other) const;
	
	
private:
	// Check whether a variant is contained within itself.
	bool NestedInSelf(const std::string &check) const;
	// Choose a ship from this variant given that it is a nested variant.
	// Nested variants only choose a single ship from among their list
	// of ships and variants.
	const Ship *NestedChooseShip() const;
	// The average credit worth of this variant if it is nested within
	// another variant.
	int64_t NestedStrength() const;
	
	
private:
	std::string name;
	std::vector<const Ship *> ships;
	WeightedList<WeightedVariant> variants;
};



#endif
