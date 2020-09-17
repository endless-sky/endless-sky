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

#include <algorithm>
#include <cmath>
#include <iterator>

using namespace std;



// Construct and Load() at the same time.
Variant::Variant(const DataNode &node)
{
	Load(node);
}



void Variant::Load(const DataNode &node, const bool global)
{
	// If this variant is global (i.e. a root node variant stored in GameData),
	// it must be named.
	if(global && node.Size() < 2)
	{
		node.PrintTrace("No name specified for variant:");
		return;
	}
	if(node.Size() >= 2 && !node.IsNumber(1))
		name = node.Token(1);
	else if(global && node.IsNumber(1))
	{
		node.PrintTrace("Variant names cannot be only numbers:");
		return;
	}
	
	// If Load() has already been called once on this variant, any subsequent
	// calls will replace the contents instead of adding to them.
	bool reset = !variants.empty() || !stockVariants.empty() || !ships.empty();
	
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
				// If toRemove is named, check if a stockVariant shares that name.
				// Else, check if toRemove is equal to any of the variants.
				if(!toRemove.Name().empty())
				{
					for(auto it = stockVariants.begin(); it != stockVariants.end(); ++it)
						if(it->first->Name() == toRemove.Name())
						{
							total -= it->second;
							variantTotal -= it->second;
							stockTotal -= it->second;
							stockVariants.erase(it);
							didRemove = true;
							break;
						}
				}
				else
				{
					for(auto it = variants.begin(); it != variants.end(); ++it)
						if(it->first == toRemove)
						{
							total -= it->second;
							variantTotal -= it->second;
							variants.erase(it);
							didRemove = true;
							break;
						}
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
						ships.erase(it);
						didRemove = true;
						--it;
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
				stockVariants.clear();
				ships.clear();
				total = 0;
				variantTotal = 0;
				stockTotal = 0;
			}
			
			int n = 1;
			if(variant)
			{
				bool named = false;
				string variantName;
				if(child.Size() >= 2 + add && !child.IsNumber(1 + add))
				{
					variantName = child.Token(1 + add);
					named = true;
					if(variantName == name)
					{
						node.PrintTrace("A variant cannot reference itself:");
						return;
					}
				}
				
				if(child.Size() >= 2 + add + named && child.Value(1 + add + named) >= 1.)
					n = child.Value(1 + add + named);
				total += n;
				variantTotal += n;
				
				// If this variant is named, then look for it in GameData.
				// Otherwise this is a new variant definition only for this variant.
				if(named)
				{
					stockVariants.emplace_back(GameData::Variants().Get(variantName), n);
					stockTotal += n;
				}
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
	{
		for(auto it = stockVariants.begin(); it != stockVariants.end(); ++it)
		{
			if(it->first->NestedInSelf(name))
			{
				total -= it->second;
				variantTotal -= it->second;
				stockTotal -= it->second;
				stockVariants.erase(it);
				--it;
				node.PrintTrace("Infinite loop detected and removed in variant \"" + name + "\":");
			}
		}
		for(auto it = variants.begin(); it != variants.end(); ++it)
		{
			if(it->first.NestedInSelf(name))
			{
				total -= it->second;
				variantTotal -= it->second;
				variants.erase(it);
				--it;
				node.PrintTrace("Infinite loop detected and removed in variant \"" + name + "\":");
			}
		}
	}
}



bool Variant::NestedInSelf(string check) const
{
	if(!name.empty() && name == check)
		return true;
	
	for(auto &it : stockVariants)
		if(it.first->NestedInSelf(check))
			return true;
	for(auto &it : variants)
		if(it.first.NestedInSelf(check))
			return true;
	
	return false;
}



const string &Variant::Name() const
{
	return name;
}



vector<const Ship *> Variant::Ships() const
{
	return ships;
}



vector<pair<const Variant *, int>> Variant::StockVariants() const
{
	return stockVariants;
}



vector<pair<Variant, int>> Variant::Variants() const
{
	return variants;
}



// Choose a list of ships from this variant. All ships from the ships
// vector are chosen, as well as a random selection of ships from any
// nested variants in the stockVariants or variants vectors.
vector<const Ship *> Variant::ChooseShips() const
{
	vector<const Ship *> chosenShips = ships;
	for(const auto &it : variants)
		for(int i = 0; i < it.second; i++)
			chosenShips.push_back(it.first.NestedChooseShip());
	for(const auto &it : stockVariants)
		for(int i = 0; i < it.second; i++)
			chosenShips.push_back(it.first->NestedChooseShip());
	return chosenShips;
}



// Choose a ship from this variant given that it is a nested variant.
// Nested variants only choose a single ship from among their list
// of ships and variants.
const Ship *Variant::NestedChooseShip() const
{
	// Randomly choose between the ships and the variants.
	if(static_cast<int>(Random::Int(total)) < variantTotal)
		return ChooseVariant(variants, stockVariants, variantTotal, stockTotal).NestedChooseShip();
	else
		return ships[Random::Int(total - variantTotal)];
}



// The strength of a variant is the sum of the cost of its ships and
// the strength of any nested variants.
int64_t Variant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	for(const auto &variant : variants)
		sum += variant.first.NestedStrength() * variant.second;
	for(const auto &variant : stockVariants)
		sum += variant.first->NestedStrength() * variant.second;
	return sum;
}



// The strength of a nested variant is its normal strength divided by
// the total weight of its contents.
int64_t Variant::NestedStrength() const
{
	return Strength() / total;
}



// A static function used by Variant and Fleet to randomly choose a single
// variant between a list of normal variants and a list of stock variants,
// given the total weight between the two and the total weight of the stock
// variants.
const Variant &Variant::ChooseVariant(const vector<pair<Variant, int>> &nVariants, const vector<pair<const Variant *, int>> &sVariants, int vTotal, int sTotal)
{
	unsigned index = 0;
	// Randomly choose between the stock variants and the non-stock variants.
	int chosen = Random::Int(vTotal);
	if(chosen < sTotal)
	{
		// "chosen" is recycled here since it's already a random int between
		// 0 and the weight of all stockVariants.
		for( ; chosen >= sVariants[index].second; ++index)
			chosen -= sVariants[index].second;
		return *sVariants[index].first;
	}
	else
	{
		for(int choice = Random::Int(vTotal - sTotal); choice >= nVariants[index].second; ++index)
			choice -= nVariants[index].second;
		return nVariants[index].first;
	}
}



// An operator for checking the equality of two unnamed variants.
bool Variant::operator==(const Variant &other) const
{
	// Are the ships of other a permutation of this variant's?
	if(other.Ships().size() != ships.size()
		|| !is_permutation(ships.begin(), ships.end(), other.Ships().begin()))
		return false;
	// Are the stockVariants of other a permutation of this variant's?
	if(other.StockVariants().size() != stockVariants.size()
		|| !is_permutation(stockVariants.begin(), stockVariants.end(), other.StockVariants().begin()))
		return false;
	// Are the nested variants of other a permutation of this variant's?
	if(other.Variants().size() != variants.size()
		|| !is_permutation(variants.begin(), variants.end(), other.Variants().begin()))
		return false;
	
	// If all checks have passed, these variants are equal.
	return true;
}
