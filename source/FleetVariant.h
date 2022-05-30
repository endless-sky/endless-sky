/* FleetVariant.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FLEET_VARIANT_H_
#define FLEET_VARIANT_H_

#include "NestedVariant.h"
#include "UnionItem.h"

#include <cstdint>
#include <vector>

class DataNode;
class Ship;



// A fleet variant represents a collection of ships that may be spawned by a
// fleet. Each variant contains one or more ships or nested variants.
class FleetVariant {
public:
	FleetVariant() = default;
	// Construct and Load() at the same time.
	FleetVariant(const DataNode &node);

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

	bool operator==(const FleetVariant &other) const;
	bool operator!=(const FleetVariant &other) const;


private:
	std::vector<const Ship *> ships;
	std::vector<UnionItem<NestedVariant>> variants;
};



#endif
