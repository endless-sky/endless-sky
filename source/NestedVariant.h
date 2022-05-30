/* NestedVariant.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef NESTED_VARIANT_H_
#define NESTED_VARIANT_H_

#include "UnionItem.h"

#include <cstdint>
#include <string>
#include <vector>

class DataNode;
class Ship;



// A nested variant represents a collection of ships that a fleet variant may
// choose from. Each nested variant contains one or more ships or nested variants.
// Nested variants may be defined as root objects and named, allowing them to be
// used across multiple fleet variants.
class NestedVariant {
public:
	NestedVariant() = default;
	// Construct and Load() at the same time.
	NestedVariant(const DataNode &node);

	void Load(const DataNode &node);

	// Determine if this nested variant template uses well-defined data.
	bool IsValid() const;

	// Choose a single ship from this nested variant. Either one ship
	// is chosen from this variant's ships list, or a ship is provided
	// by one of this variant's nested variants.
	const Ship *ChooseShip() const;

	// The strength of a nested variant is the sum of the cost of its ships
	// the strength of any nested variants divided by the total number of
	// ships and variants.
	int64_t Strength() const;

	bool operator==(const NestedVariant &other) const;
	bool operator!=(const NestedVariant &other) const;


private:
	// Check whether a nested variant is contained within itself.
	bool NestedInSelf(const std::string &check) const;


private:
	std::string name;
	std::vector<const Ship *> ships;
	std::vector<UnionItem<NestedVariant>> variants;
};



#endif
