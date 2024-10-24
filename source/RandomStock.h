/* RandomStock.h
Copyright (c) 2024 by David Stark (Zarkonnen)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef RANDOM_STOCK_H_
#define RANDOM_STOCK_H_

#include "DataNode.h"
#include "Depreciation.h"
#include "Set.h"

#include <set>
#include <string>



// Struct representing an item that may sometimes be in stock.
template <class Item>
struct RandomStockItem
{
	const Item *item;
	// Probability this item is in stock, as a percentage.
	unsigned int probability = 100;
	// The number of such a set of items in stock.
	unsigned int quantity = 1;
	// Days of depreciation.
	unsigned int depreciation = 0;
};



// Class representing a set of items that is sometimes in stock.
template <class Item>
class RandomStock : public std::list<RandomStockItem<Item>>
{
public:
	void Load(const DataNode &node, const Set<Item> &items);
};



template <class Item>
void RandomStock<Item>::Load(const DataNode &node, const Set<Item> &items)
{
	for(const DataNode &child : node)
	{
		const std::string &token = child.Token(0);
		bool remove = (token == "clear" || token == "remove");
		if(remove && child.Size() == 1)
			this->clear();
		else if(remove && child.Size() >= 2)
		{
			const auto it = items.Get(child.Token(1));
			this->remove_if([&it](RandomStockItem<Item> o){ return o.item == it; });
		}
		else
		{
			RandomStockItem<Item> rs = { items.Get(child.Token(token == "add" ? 1 : 0)) };

			for(const DataNode &grand : child)
			{
				const std::string &grandToken = grand.Token(0);
				if(grandToken == "probability")
					rs.probability = grand.Value(1);
				if(grandToken == "quantity")
					rs.quantity = grand.Value(1);
				if(grandToken == "depreciation")
					rs.depreciation = grand.Value(1);
				if(grandToken == "discount")
					rs.depreciation = Depreciation::AgeForDepreciation(1 - grand.Value(1) / 100.0);
			}

			this->push_back(rs);
		}
	}
}



#endif
