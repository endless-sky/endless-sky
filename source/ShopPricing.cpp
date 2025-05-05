/* ShopPricing.cpp
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

#include "ShopPricing.h"

#include "DataNode.h"

#include <cmath>

using namespace std;



ShopPricing::ShopPricing(const DataNode &node)
{
	Load(node);
}



void ShopPricing::Load(const DataNode &node)
{
	isLoaded = true;

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		const bool hasValue = child.Size() > 1;
		if(key == "ignore depreciation")
			ignoreDepreciation = true;
		else if(key == "multiplier" && hasValue)
			multiplier = max<double>(0., child.Value(1));
		else if(key == "offset" && hasValue)
			offset = child.Value(1);
		else if(key == "precedence" && hasValue)
			precedence = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool ShopPricing::IsLoaded() const
{
	return isLoaded;
}



void ShopPricing::Combine(const ShopPricing &other)
{
	if(other.precedence < precedence)
		return;

	if(other.precedence > precedence)
	{
		precedence = other.precedence;
		multiplier = other.multiplier;
		offset = other.offset;
		ignoreDepreciation = other.ignoreDepreciation;
		return;
	}

	multiplier *= other.multiplier;
	offset += other.offset;
	ignoreDepreciation |= other.ignoreDepreciation;
}



int64_t ShopPricing::Value(int64_t cost, double depreciation, int count) const
{
	int64_t value = cost * multiplier + offset;
	// If the offset caused the value to go negative, return 0.
	if(value <= 0)
		return 0;
	// If ignoring depreciation, return the base value multiplied by the item count.
	if(ignoreDepreciation)
		return value * count;
	// If depreciation is applied, the provided depreciation fraction will already
	// account for the item count.
	return value * depreciation;
}
