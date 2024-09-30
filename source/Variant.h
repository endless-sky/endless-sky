/* Variant.h
Copyright (c) 2022 by Amazinite

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
#include <vector>

class DataNode;
class Ship;


// A variant represents a collection of ships that may be spawned by a fleet.
// Each variant contains one or more ships.
class Variant {
public:
	Variant() = default;
	// Construct and Load() at the same time.
	explicit Variant(const DataNode &node);

	void Load(const DataNode &node);

	// Determine if this variant template uses well-defined data.
	bool IsValid() const;

	int Weight() const;
	const std::vector<const Ship *> &Ships() const;

	// The strength of a variant is the sum of the cost of its ships.
	int64_t Strength() const;

	bool operator==(const Variant &other) const;
	bool operator!=(const Variant &other) const;


private:
	int weight = 1;
	std::vector<const Ship *> ships;
};
