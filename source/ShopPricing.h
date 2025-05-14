/* ShopPricing.h
Copyright (c) 2025 by Amazinite

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

class DataNode;



// Class representing price modifications to items in a shop.
class ShopPricing {
public:
	ShopPricing() noexcept = default;
	ShopPricing(const DataNode &node);

	void Load(const DataNode &node);
	bool IsLoaded() const;

	// Combine this price modifier with a different price modifier.
	// - If the other modifier's precedence is lower than this one, do nothing.
	// - If the other modifier's precedence is higher than this one, take on its values.
	// - If the precedences match:
	//   - Multipliers are multiplied together.
	//   - Offsets are added together.
	//   - Depreciation is ignored if either modifier ignores depreciation.
	void Combine(const ShopPricing &other);

	// Calculate the value of an item according to this modifier.
	int64_t Value(int64_t cost, int count, double depreciation) const;


private:
	bool isLoaded = false;

	// A multiplier applied to the cost of an item.
	double multiplier = 1.;
	// An offset that can be added to or subtracted from the cost.
	// This is applied after the multiplier.
	int64_t offset = 0;
	// If true, the reduced cost incurred by depreciation is ignored.
	bool ignoreDepreciation = false;
	// A value used to determine which pricing modifiers should be
	// applied to an outfit if there are multiple modifiers present.
	// Modifiers with the same precedence value are combined. The modifier
	// with the highest precedence is then what gets used.
	int precedence = 0;
};
