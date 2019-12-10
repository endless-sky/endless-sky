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

namespace {
}



// Construct and Load() at the same time.
Variant::Variant(const DataNode &node)
{
	Load(node);
}



void Variant::Load(const DataNode &node, const bool global)
{
	if(global)
	{
		if(node.Size() < 2)
		{
			node.PrintTrace("No name specified for variant:");
			return;
		}
		name = node.Token(1);
	}
	
	for(const DataNode &child : node)
	{
		bool variant = (child.Token(0) == "variant");
		int n = 1;
		
		if(variant)
		{
			bool named = false;
			string variantName;
			if(child.Size() >= 2 && !child.IsNumber(1))
			{
				variantName = child.Token(1);
				named = true;
				if(variantName == name)
				{
					node.PrintTrace("A variant can not reference itself:");
					return;
				}
			}
			
			if(child.Size() >= 2 + named)
				n = child.Value(1 + named);
			total += n;
			variantTotal += n;
			
			if(named)
				variants.emplace_back(make_pair(GameData::Variants().Get(variantName), n));
			else
				variants.emplace_back(make_pair(new Variant(child), n));
		}
		else
		{
			if(child.Size() >= 2 && child.Value(1) >= 1.)
				n = child.Value(1);
			total += n;
			shipTotal += n;
			ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(0)));
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



vector<pair<const Variant *, int>> Variant::Variants() const
{
	return variants;
}



vector<const Ship *> Variant::ChooseShips() const
{
	vector<const Ship *> chosenShips = ships;
	for(auto &it : variants)
	{
		for(int i = 0; i < it.second; i++)
		{
			vector<const Ship *> variantShips = it.first->NestedChooseShips();
			chosenShips.insert(chosenShips.end(), variantShips.begin(), variantShips.end());
		}
	}
	return chosenShips;
}



vector<const Ship *> Variant::NestedChooseShips() const
{
	vector<const Ship *> chosenShips;
	
	int chosen = Random::Int(total);
	if(chosen < variantTotal)
	{
		unsigned variantIndex = 0;
		for(int choice = Random::Int(variantTotal); choice >= variants[variantIndex].second; ++variantIndex)
			choice -= variants[variantIndex].second;
		
		vector<const Ship *> variantShips = variants[variantIndex].first->NestedChooseShips();
		chosenShips.insert(chosenShips.end(), variantShips.begin(), variantShips.end());
	}
	else
		chosenShips.push_back(ships[Random::Int(shipTotal)]);
	
	return chosenShips;
}



int64_t Variant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	for(auto variant : variants)
		sum += variant.first->NestedStrength() * variant.second;
	return sum;
}



int64_t Variant::NestedStrength() const
{
	return Strength() / (ships.size() + variants.size());
}
