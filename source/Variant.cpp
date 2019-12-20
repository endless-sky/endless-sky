/* Variant.cpp
Copyright (c) 2019 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Fleet.h"

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
	if(global && node.Size() < 2)
	{
		node.PrintTrace("No name specified for variant:");
		return;
	}
	if(node.Size() >= 2 && !node.IsNumber(1))
		name = node.Token(1);
	
	bool reset = !variants.empty() || !stockVariants.empty() || !ships.empty();
	
	for(const DataNode &child : node)
	{
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		bool variant = (child.Token(add || remove) == "variant");
		
		if(remove)
		{
			bool didRemove = false;
			if(variant)
			{
				// If given a full definition of one of this fleet's variant members, remove the variant.
				Variant toRemove(child);
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
				
				if(!didRemove)
					for(auto it = variants.begin(); it != variants.end(); ++it)
						if(it->first.Equals(toRemove))
						{
							total -= it->second;
							variantTotal -= it->second;
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
				
				if(named)
				{
					stockVariants.emplace_back(make_pair(GameData::Variants().Get(variantName), n));
					stockTotal += n;
				}
				else
					variants.emplace_back(make_pair(Variant(child), n));
			}
			else
			{
				if(child.Size() >= 2 + add && child.Value(1 + add) >= 1.)
					n = child.Value(1 + add);
				total += n;
				ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(0)));
			}
		}
	}
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



vector<const Ship *> Variant::ChooseShips() const
{
	vector<const Ship *> chosenShips = ships;
	for(auto &it : variants)
		for(int i = 0; i < it.second; i++)
		{
			vector<const Ship *> variantShips = it.first.NestedChooseShips();
			chosenShips.insert(chosenShips.end(), variantShips.begin(), variantShips.end());
		}
	for(auto &it : stockVariants)
		for(int i = 0; i < it.second; i++)
		{
			vector<const Ship *> variantShips = it.first->NestedChooseShips();
			chosenShips.insert(chosenShips.end(), variantShips.begin(), variantShips.end());
		}
	return chosenShips;
}



vector<const Ship *> Variant::NestedChooseShips() const
{
	vector<const Ship *> chosenShips;
	
	int chosen = Random::Int(total);
	if(chosen < variantTotal)
	{
		chosen = Random::Int(variantTotal);
		unsigned variantIndex = 0;
		vector<const Ship *> variantShips;
		if(chosen < stockTotal)
		{
			for(int choice = Random::Int(stockTotal); choice >= stockVariants[variantIndex].second; ++variantIndex)
				choice -= stockVariants[variantIndex].second;
			
			variantShips = stockVariants[variantIndex].first->NestedChooseShips();
		}
		else
		{
			for(int choice = Random::Int(variantTotal - stockTotal); choice >= variants[variantIndex].second; ++variantIndex)
				choice -= variants[variantIndex].second;
			
			variantShips = variants[variantIndex].first.NestedChooseShips();
		}
		chosenShips.insert(chosenShips.end(), variantShips.begin(), variantShips.end());
	}
	else
		chosenShips.push_back(ships[Random::Int(total - variantTotal)]);
	
	return chosenShips;
}



int64_t Variant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	for(auto &variant : variants)
		sum += variant.first.NestedStrength() * variant.second;
	for(auto &variant : stockVariants)
		sum += variant.first->NestedStrength() * variant.second;
	return sum;
}



int64_t Variant::NestedStrength() const
{
	return Strength() / total;
}



bool Variant::Equals(Variant toRemove) const
{
	// Are the ships of toRemove a permutation of this variant?
	if(toRemove.Ships().size() != ships.size()
		|| !is_permutation(ships.begin(), ships.end(), toRemove.Ships().begin()))
		return false;
	// Are the stockVariants of toRemove a permutation of this variant?
	if(toRemove.StockVariants().size() != stockVariants.size()
		|| !is_permutation(stockVariants.begin(), stockVariants.end(), toRemove.StockVariants().begin()))
		return false;
	// Does toRemove have the same number of nested variants as this variant?
	if(toRemove.Variants().size() != variants.size())
		return false;
	// Are the variants of toRemove equal to the variants of this variant?
	for(auto variant : variants)
	{
		// For each nested variant in this variant, look for a matching nested variant in toRemove.
		bool sameVariants = false;
		for(auto toRemoveVariant : toRemove.Variants())
		{
			// If a matching nested variant was found, break out of this loop.
			if(variant.first.Equals(toRemoveVariant.first));
			{
				sameVariants = true;
				break;
			}
		}
		// If no matching variant in toRemove was found for this nested variant, then toRemove and this variant are not equal.
		if(!sameVariants)
			return false;
	}
	
	// If all checks have passed, these variants are equal.
	return true;
}
