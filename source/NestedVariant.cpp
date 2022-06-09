/* NestedVariant.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "NestedVariant.h"

#include "DataNode.h"
#include "GameData.h"
#include "Random.h"
#include "Ship.h"

#include <algorithm>

using namespace std;



// Construct and Load() at the same time.
NestedVariant::NestedVariant(const DataNode &node)
{
	Load(node);
}



void NestedVariant::Load(const DataNode &node)
{
	// If this variant is being loaded from a fleet variant, it may include
	// an additional token that shifts where the name must be searched for.
	bool addNode = (node.Token(0) == "add");
	bool removeNode = (node.Token(0) == "remove");
	// If this variant is being loaded with a second token that is not a number,
	// then it's a name that must be saved. Account for the shift in index of the
	// name caused by a possible "remove" token. If a variant is being added and
	// loaded then it shouldn't be named.
	if(!addNode && node.Size() >= 2 + removeNode && !node.IsNumber(1 + removeNode))
	{
		name = node.Token(1 + removeNode);
		// If this named variant is being loaded for removal purposes then
		// all that is necessary is that the variant has its name.
		if(removeNode)
			return;
	}

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
			if(variant && !variants.empty())
			{
				// If given a full definition of a nested variant, remove all instances of that
				// nested variant from this variant.
				auto removeIt = std::remove(variants.begin(), variants.end(), UnionItem<NestedVariant>(child));
				if(removeIt != variants.end())
					variants.erase(removeIt, variants.end());
				else
					child.PrintTrace("Warning: Did not find matching variant for specified operation:");
			}
			else if(!ships.empty())
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
				{
					variantName = child.Token(index++);
					if(variantName == name)
					{
						child.PrintTrace("Warning: A variant cannot reference itself:");
						continue;
					}
				}

				if(child.Size() >= index + 1 && child.Value(index) >= 1.)
					n = child.Value(index);

				// If this variant is named, then look for it in GameData.
				// Otherwise this is a new variant definition only for this variant.
				if(!variantName.empty())
				{
					variants.insert(variants.end(), n, ExclusiveItem<NestedVariant>(GameData::Variants().Get(variantName)));
					if(child.HasChildren())
						child.PrintTrace("Warning: Skipping children of named variant in variant definition:");
				}
				else
					variants.insert(variants.end(), n, ExclusiveItem<NestedVariant>(child));
			}
			else
			{
				if(child.Size() >= index + 1 && child.Value(index) >= 1.)
					n = child.Value(index);
				ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(index - 1)));
			}
		}
	}

	// Prevent a named variant from containing itself. Even if the nested variants
	// of this variant aren't loaded yet, eventually the loop will be found after
	// the last variant loads.
	if(!name.empty())
	{
		auto removeIt = remove_if(variants.begin(), variants.end(),
			[this](const ExclusiveItem<NestedVariant> &v) noexcept -> bool { return v->NestedInSelf(name); });
		if(removeIt != variants.end())
		{
			variants.erase(removeIt, variants.end());
			node.PrintTrace("Error: Infinite loop detected and removed in variant \"" + name + "\".");
		}
	}
}



// Determine if this nested variant template uses well-defined data.
bool NestedVariant::IsValid() const
{
	// All possible ships must be valid.
	if(any_of(ships.begin(), ships.end(),
			[](const Ship *const s) noexcept -> bool { return !s->IsValid(); }))
		return false;

	// All possible nested variants must be valid.
	if(any_of(variants.begin(), variants.end(),
			[](const UnionItem<NestedVariant> &v) noexcept -> bool { return !v->IsValid(); }))
		return false;

	return true;
}



// Choose a single ship from this nested variant. Either one ship
// is chosen from this variant's ships list, or a ship is provided
// by one of this variant's nested variants.
const Ship *NestedVariant::ChooseShip() const
{
	// Randomly choose between the ships and the variants.
	if(Random::Int(ships.size() + variants.size()) < variants.size())
		return variants[Random::Int(variants.size())]->ChooseShip();

	return ships[Random::Int(ships.size())];
}



// The strength of a nested variant is the sum of the cost of its ships
// the strength of any nested variants divided by the total number of
// ships and variants.
int64_t NestedVariant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	for(const auto &variant : variants)
		sum += variant->Strength();
	return sum / (ships.size() + variants.size());
}



bool NestedVariant::operator==(const NestedVariant &other) const
{
	// Is either variant named? Do they share the same name?
	if(!other.name.empty() || !name.empty())
		return other.name == name;

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



bool NestedVariant::operator!=(const NestedVariant &other) const
{
	return !(*this == other);
}



// Check whether a variant is contained within itself.
bool NestedVariant::NestedInSelf(const string &check) const
{
	if(!name.empty() && name == check)
		return true;

	for(const auto &it : variants)
		if(it->NestedInSelf(check))
			return true;

	return false;
}
