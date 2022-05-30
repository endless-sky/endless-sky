/* FleetVariant.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FleetVariant.h"

#include "DataNode.h"
#include "GameData.h"
#include "Ship.h"

#include <algorithm>

using namespace std;



// Construct and Load() at the same time.
FleetVariant::FleetVariant(const DataNode &node)
{
	Load(node);
}



void FleetVariant::Load(const DataNode &node)
{
	// If Load() has already been called once on this variant, any subsequent
	// calls will replace the contents instead of adding to them.
	bool reset = !variants.empty() || !ships.empty();

	for(const DataNode &child : node)
	{
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		if((add || remove) && child.Size() == 1)
		{
			child.PrintTrace("Skipping invalid \"" + child.Token(0) + "\" tag:");
			continue;
		}
		bool variant = (child.Token(add || remove) == "variant");

		if(remove)
		{
			if(variant)
			{
				// If given a full definition of a nested variant, remove all instances of that
				// nested variant from this variant.
				auto removeIt = std::remove(variants.begin(), variants.end(), UnionItem<NestedVariant>(child));
				if(removeIt != variants.end())
					variants.erase(removeIt, variants.end());
				else
					child.PrintTrace("Warning: Did not find matching variant for specified operation:");
			}
			else
			{
				// If given the name of a ship, remove all instances of that ship from this variant.
				auto removeIt = std::remove(ships.begin(), ships.end(), GameData::Ships().Get(child.Token(1)));
				if(removeIt != ships.end())
					ships.erase(removeIt, ships.end());
				else
					child.PrintTrace("Warning: Did not find matching ship for specified operation:");
			}
		}
		else
		{
			// If this is a subsequent call of Load(), clear the variant
			// if we aren't adding to it.
			if(reset && !add)
			{
				reset = false;
				variants.clear();
				ships.clear();
			}

			int n = 1;
			int index = 1 + add;
			if(variant)
			{
				string variantName;
				if(child.Size() >= index + 1 && !child.IsNumber(index))
					variantName = child.Token(index++);

				if(child.Size() >= index + 1 && child.Value(index) >= 1.)
					n = child.Value(index);

				// If this nested variant is named, then look for it in GameData.
				// Otherwise this is a new nested variant definition only for this
				// fleet variant.
				if(!variantName.empty())
				{
					variants.insert(variants.end(), n, UnionItem<NestedVariant>(GameData::Variants().Get(variantName)));
					if(child.HasChildren())
						child.PrintTrace("Warning: Skipping children of named variant in variant definition:");
				}
				else
					variants.insert(variants.end(), n, UnionItem<NestedVariant>(child));
			}
			else
			{
				if(child.Size() >= index + 1 && child.Value(index) >= 1.)
					n = child.Value(index);
				ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(index - 1)));
			}
		}
	}
}



// Determine if this fleet variant template uses well-defined data.
bool FleetVariant::IsValid() const
{
	// At least one valid ship is enough to make the variant valid.
	if(any_of(ships.begin(), ships.end(),
			[](const Ship *const s) noexcept -> bool { return s->IsValid(); }))
		return true;

	// At least one nested variant is enough to make the variant valid.
	if(any_of(variants.begin(), variants.end(),
			[](const UnionItem<NestedVariant> &v) noexcept -> bool { return v.GetItem().IsValid(); }))
		return true;

	return false;
}



// Choose a list of ships from this variant. All ships from the ships
// vector are chosen, as well as a random selection of ships from any
// nested variants in the WeightedList.
vector<const Ship *> FleetVariant::ChooseShips() const
{
	vector<const Ship *> chosenShips = ships;
	for(const auto &it : variants)
		chosenShips.push_back(it.GetItem().ChooseShip());
	return chosenShips;
}



// The strength of a variant is the sum of the cost of its ships and
// the strength of any nested variants.
int64_t FleetVariant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	for(const auto &variant : variants)
		sum += variant.GetItem().Strength();
	return sum;
}



bool FleetVariant::operator==(const FleetVariant &other) const
{
	// Are the ships of other a permutation of this variant's?
	if(other.ships.size() != ships.size()
		|| !is_permutation(ships.begin(), ships.end(), other.ships.begin()))
		return false;
	// Are the nested variants of other a permutation of this variant's?
	if(other.variants.size() != variants.size()
		|| !is_permutation(variants.begin(), variants.end(), other.variants.begin()))
		return false;

	// If all checks have passed, these variants are equal.
	return true;
}



bool FleetVariant::operator!=(const FleetVariant &other) const
{
	return !(*this == other);
}
