/* Variant.cpp
Copyright (c) 2019 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Variant.h"

#include "DataNode.h"
#include "GameData.h"
#include "Random.h"
#include "Ship.h"
#include "WeightedVariant.h"

#include <algorithm>
#include <cmath>
#include <iterator>

using namespace std;



// Construct and Load() at the same time.
Variant::Variant(const DataNode &node)
{
	Load(node);
}



void Variant::Load(const DataNode &node)
{
	// If this variant is being loaded with a second token that is not a number,
	// then it must be being loaded from GameData, and therefore must have its
	// name saved.
	if(node.Size() >= 2 && !node.IsNumber(1))
		name = node.Token(1);
	
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
			bool didRemove = false;
			if(variant)
			{
				// If given a full definition of one of this fleet's variant members, remove the variant.
				Variant toRemove(child);
				for(auto it = variants.begin(); it != variants.end(); ++it)
					if(it->Get() == toRemove)
					{
						total -= it->Weight();
						variants.erase(it);
						didRemove = true;
						break;
					}
				
				if(!didRemove)
					child.PrintTrace("Did not find matching variant for specified operation:");
			}
			else
			{
				// If given the name of a ship, remove all ships by that name from this variant.
				string shipName = child.Token(1);
				for(auto it = ships.begin(); it != ships.end(); ++it)
					if((*it)->ModelName() == shipName)
					{
						--total;
						it = ships.erase(it);
						didRemove = true;
					}
				
				if(!didRemove)
					child.PrintTrace("Did not find matching ship for specified operation:");
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
				total = 0;
			}
			
			int n = 1;
			if(variant)
			{
				string variantName;
				if(child.Size() >= 2 + add && !child.IsNumber(1 + add))
				{
					variantName = child.Token(1 + add);
					if(variantName == name)
					{
						node.PrintTrace("A variant cannot reference itself:");
						return;
					}
				}
				
				if(child.Size() >= 2 + add + !variantName.empty() && child.Value(1 + add + !variantName.empty()) >= 1.)
					n = child.Value(1 + add + !variantName.empty());
				total += n;
				
				// If this variant is named, then look for it in GameData.
				// Otherwise this is a new variant definition only for this variant.
				if(!variantName.empty())
					variants.emplace_back(GameData::Variants().Get(variantName), n);
				else
					variants.emplace_back(child, n);
			}
			else
			{
				if(child.Size() >= 2 + add && child.Value(1 + add) >= 1.)
					n = child.Value(1 + add);
				total += n;
				ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(add)));
			}
		}
	}

	// Prevent a variant from containing itself.
	if(!name.empty())
		for(auto it = variants.begin(); it != variants.end(); ++it)
			if(it->Get().NestedInSelf(name))
			{
				total -= it->Weight();
				it = variants.erase(it);
				node.PrintTrace("Infinite loop detected and removed in variant \"" + name + "\":");
			}
}



bool Variant::NestedInSelf(string check) const
{
	if(!name.empty() && name == check)
		return true;
	
	for(const auto &it : variants)
		if(it.Get().NestedInSelf(check))
			return true;
	
	return false;
}



bool Variant::IsValid() const
{
	// A variant should have at least one valid ship
	// or nested variant.
	if(none_of(ships.begin(), ships.end(),
			[](const Ship *const s) noexcept -> bool { return s->IsValid(); })
		&& none_of(variants.begin(), variants.end(),
			[](const WeightedVariant &v) noexcept -> bool {return v.Get().IsValid()))
		return false;
	
	return true;
}



const string &Variant::Name() const
{
	return name;
}



const vector<const Ship *> &Variant::Ships() const
{
	return ships;
}



const WeightedList<WeightedVariant> &Variant::Variants() const
{
	return variants;
}



// Choose a list of ships from this variant. All ships from the ships
// vector are chosen, as well as a random selection of ships from any
// nested variants in the WeightedList.
vector<const Ship *> Variant::ChooseShips() const
{
	vector<const Ship *> chosenShips = ships;
	for(const auto &it : variants)
		for(int i = 0; i < it.Weight(); i++)
			chosenShips.push_back(it.Get().NestedChooseShip());
	return chosenShips;
}



// Choose a ship from this variant given that it is a nested variant.
// Nested variants only choose a single ship from among their list
// of ships and variants.
const Ship *Variant::NestedChooseShip() const
{
	// Randomly choose between the ships and the variants.
	if(static_cast<int>(Random::Int(total)) < static_cast<int>(variants.TotalWeight()))
		return variants.Get().Get().NestedChooseShip();
	else
		return ships[Random::Int(total - variants.TotalWeight())];
}



// The strength of a variant is the sum of the cost of its ships and
// the strength of any nested variants.
int64_t Variant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	for(const auto &variant : variants)
		sum += variant.Get().NestedStrength() * variant.Weight();
	return sum;
}



// The strength of a nested variant is its normal strength divided by
// the total weight of its contents.
int64_t Variant::NestedStrength() const
{
	return Strength() / total;
}



// An operator for checking the equality of two variants.
bool Variant::operator==(const Variant &other) const
{
	// Are one of the variants named but not the other?
	if((other.Name().empty() && !name.empty())
		|| (!other.Name().empty() && name.empty()))
		return false;
	
	// Are both variants named and share the same name?
	if(!other.Name().empty() && !name.empty())
		return other.Name() == name;
	
	// Are the ships of other a permutation of this variant's?
	if(other.Ships().size() != ships.size()
		|| !is_permutation(ships.begin(), ships.end(), other.Ships().begin()))
		return false;
	// Are the nested variants of other a permutation of this variant's?
	if(other.Variants().size() != variants.size()
		|| !is_permutation(variants.begin(), variants.end(), other.Variants().begin()))
		return false;
	
	// If all checks have passed, these variants are equal.
	return true;
}
