/* OrderSet.h
Copyright (c) 2024 by TomGoodIdea

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

#include "OrderSingle.h"

#include <bitset>



// This class holds a combination of orders given to a ship.
class OrderSet : public Orders {
public:
	// Operations on the bitset
	bool Has(Types type) const noexcept;
	bool Empty() const noexcept;

	// Add a single new order to this set.
	void Add(const OrderSingle &newOrder, bool *hasMismatch = nullptr, bool *alreadyHarvesting = nullptr);
	// Remove orders that need a ship/asteroid target if the current target is invalid.
	void Validate(const Ship *ship, const System *playerSystem);
	// Update the internal variants of the "hold position" order.
	void Update(const Ship &ship);


private:
	void Set(Types type) noexcept;
	void Reset(Types type) noexcept;


private:
	std::bitset<static_cast<size_t>(Types::TYPES_COUNT)> types;
};
