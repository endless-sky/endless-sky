/* Variant.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Variant.h"

#include "DataNode.h"
#include "Files.h"
#include "GameData.h"
#include "Random.h"
#include "Ship.h"

#include <algorithm>
#include <iterator>

using namespace std;



// Construct and Load() at the same time.
Variant::Variant(const DataNode &node)
{
	Load(node);
}



void Variant::Load(const DataNode &node)
{
	// If this variant is being loaded from a fleet or another variant, it may
	// include an additional token that shifts where the name must be searched for.
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
			if(variant)
			{
				// If given a full definition of one of this variant's variant members, remove the variant.
				Variant toRemove(child);
				auto removeIt = remove_if(variants.begin(), variants.end(),
					[&toRemove](const UnionItem<Variant> &v) noexcept -> bool { return v.GetItem() == toRemove; });
				if(removeIt != variants.end())
					variants.erase(removeIt, variants.end());
				else
					child.PrintTrace("Warning: Did not find matching variant for specified operation:");
			}
			else
			{
				// If given the name of a ship, remove all ships by that name from this variant.
				string shipName = child.Token(1);
				auto removeIt = remove_if(ships.begin(), ships.end(),
					[&shipName](const Ship *s) noexcept -> bool { return s->VariantName() == shipName; });
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
						node.PrintTrace("Warning: A variant cannot reference itself:");
						continue;
					}
				}

				if(child.Size() >= index + 1 && child.Value(index) >= 1.)
					n = child.Value(index);

				// If this variant is named, then look for it in GameData.
				// Otherwise this is a new variant definition only for this variant.
				if(!variantName.empty())
				{
					variants.insert(variants.end(), n, UnionItem<Variant>(GameData::Variants().Get(variantName)));
					if(child.HasChildren())
						child.PrintTrace("Warning: Skipping children of named variant in variant definition:");
				}
				else
					variants.insert(variants.end(), n, UnionItem<Variant>(Variant(child)));
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
		for(auto it = variants.begin(); it != variants.end(); )
		{
			if(it->GetItem().NestedInSelf(name))
			{
				it = variants.erase(it);
				Files::LogError("Error: Infinite loop detected and removed in variant \"" + name + "\".");
			}
			else
				++it;
		}
}



// Determine if this variant template uses well-defined data.
bool Variant::IsValid() const
{
	// At least one valid ship is enough to make the variant valid.
	if(any_of(ships.begin(), ships.end(),
			[](const Ship *const s) noexcept -> bool { return s->IsValid(); }))
		return true;

	// At least one nested variant is enough to make the variant valid.
	if(any_of(variants.begin(), variants.end(),
			[](const UnionItem<Variant> &v) noexcept -> bool { return v.GetItem().NestedIsValid(); }))
		return true;

	return false;
}



// Choose a list of ships from this variant. All ships from the ships
// vector are chosen, as well as a random selection of ships from any
// nested variants in the WeightedList.
vector<const Ship *> Variant::ChooseShips() const
{
	vector<const Ship *> chosenShips = ships;
	for(const auto &it : variants)
		chosenShips.push_back(it.GetItem().NestedChooseShip());
	return chosenShips;
}



// The strength of a variant is the sum of the cost of its ships and
// the strength of any nested variants.
int64_t Variant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	for(const auto &variant : variants)
		sum += variant.GetItem().NestedStrength();
	return sum;
}



// An operator for checking the equality of two variants.
bool Variant::operator==(const Variant &other) const
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



// An operator for checking the inequality of two variants.
bool Variant::operator!=(const Variant &other) const
{
	return !(*this == other);
}



// Check whether a variant is contained within itself.
bool Variant::NestedInSelf(const string &check) const
{
	if(!name.empty() && name == check)
		return true;

	for(const auto &it : variants)
		if(it.GetItem().NestedInSelf(check))
			return true;

	return false;
}



// Determine if this variant template uses well-defined data as a
// nested variant.
bool Variant::NestedIsValid() const
{
	// All possible ships must be valid.
	if(any_of(ships.begin(), ships.end(),
			[](const Ship *const s) noexcept -> bool { return !s->IsValid(); }))
		return false;

	// All possible nested variants must be valid.
	if(any_of(variants.begin(), variants.end(),
			[](const UnionItem<Variant> &v) noexcept -> bool { return !v.GetItem().NestedIsValid(); }))
		return false;

	return true;
}



// Choose a ship from this variant given that it is a nested variant.
// Nested variants only choose a single ship from among their list
// of ships and variants.
const Ship *Variant::NestedChooseShip() const
{
	// Randomly choose between the ships and the variants.
	if(Random::Int(ships.size() + variants.size()) < variants.size())
		return variants[Random::Int(variants.size())].GetItem().NestedChooseShip();

	return ships[Random::Int(ships.size())];
}



// The strength of a nested variant is its normal strength divided by
// the total weight of its contents.
int64_t Variant::NestedStrength() const
{
	return Strength() / (ships.size() + variants.size());
}
