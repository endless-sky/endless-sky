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

namespace
{
}



// Construct and Load() at the same time.
Variant::Variant(const DataNode &node)
{
	Load(node);
}



void Variant::Load(const DataNode &node, const bool global)
{
	if(!global)
	{
		weight = 1;
		if(node.Token(0) == "variant" && node.Size() >= 2)
			weight = node.Value(1);
		else if(node.Token(0) == "add" && node.Size() >= 3)
			weight = node.Value(2);
	}
	else
	{
	}
	
	for(const DataNode &child : node)
	{
		int n = 1;
		if(child.Size() >= 2 && child.Value(1) >= 1.)
			n = child.Value(1);
		ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(0)));
	}
}



int Variant::Weight() const
{
	return weight;
}



vector<const Ship *> Variant::Ships() const
{
	return ships;
}
