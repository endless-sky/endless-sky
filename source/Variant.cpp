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
		int n = 1;
		if(child.Size() >= 2 && child.Value(1) >= 1.)
			n = child.Value(1);
		ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(0)));
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



int64_t Variant::Strength() const
{
	int64_t sum = 0;
	for(const Ship *ship : ships)
		sum += ship->Cost();
	return sum;
}
