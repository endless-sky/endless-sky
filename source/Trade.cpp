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

#include "DataNode.h"

#include <algorithm>
#include <cassert>

using namespace std;



void Trade::Load(const DataNode &node)
{
	assert(node.Token(0) == "trade");
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "commodity" && child.Size() >= 4)
		{
			auto it = commodities.begin();
			for( ; it != commodities.end(); ++it)
				if(it->name == child.Token(1))
					break;
			if(it == commodities.end())
				it = commodities.insert(it, Commodity());
			
			it->name = child.Token(1);
			it->low = child.Value(2);
			it->high = child.Value(3);
			for(const DataNode &grand : child)
				it->items.push_back(grand.Token(0));
		}
		else if(child.Token(0) == "clear")
			commodities.clear();
	}
}



const vector<Trade::Commodity> &Trade::Commodities() const
{
	return commodities;
}
