/* Trade.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Trade.h"

#include "DataNode.h"

#include <algorithm>

using namespace std;



void Trade::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "commodity" && child.Size() >= 2)
		{
			bool isSpecial = (child.Size() < 4);
			vector<Commodity> &list = (isSpecial ? specialCommodities : commodities);
			auto it = list.begin();
			for( ; it != list.end(); ++it)
				if(it->name == child.Token(1))
					break;
			if(it == list.end())
				it = list.insert(it, Commodity());

			it->name = child.Token(1);
			if(!isSpecial)
			{
				it->low = child.Value(2);
				it->high = child.Value(3);
			}
			for(const DataNode &grand : child)
				it->items.push_back(grand.Token(0));
		}
		else if(key == "clear")
			commodities.clear();
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const vector<Trade::Commodity> &Trade::Commodities() const
{
	return commodities;
}



const vector<Trade::Commodity> &Trade::SpecialCommodities() const
{
	return specialCommodities;
}
