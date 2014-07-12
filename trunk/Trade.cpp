/* Trade.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Trade.h"

#include <cassert>

using namespace std;



void Trade::Load(const DataFile::Node &node)
{
	assert(node.Token(0) == "trade");
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "commodity" && child.Size() >= 4)
		{
			commodities.push_back(Commodity());
			commodities.back().name = child.Token(1);
			commodities.back().low = child.Value(2);
			commodities.back().high = child.Value(3);
			for(const DataFile::Node &grand : child)
				commodities.back().items.push_back(grand.Token(0));
		}
	}
}



const vector<Trade::Commodity> &Trade::Commodities() const
{
	return commodities;
}
