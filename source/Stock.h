/* Stock.h
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "StockItem.h"

#include <algorithm>
#include <set>



// Class representing a set of items that are for sale on a given planet.
// Multiple Stock sets can be merged together into a single one.
template <class Item>
class Stock : public std::set<StockItem<Item>> {
public:
	void Add(const Stock<Item> &other);
	bool Has(const Item *item) const;
	const StockItem<Item> *Get(const Item *item) const;
};



template <class Item>
void Stock<Item>::Add(const Stock<Item> &other)
{
	// When adding two Stock sets together, items that exist
	// in both sets are combined.
	for(const StockItem<Item> &item : other)
	{
		auto node = this->extract(item);
		if(!node.empty())
		{
			node.value().Combine(item);
			this->insert(std::move(node));
		}
		else
			this->insert(item);
	}
}



template <class Item>
bool Stock<Item>::Has(const Item *item) const
{
	return find(this->begin(), this->end(), item) != this->end();
}



template <class Item>
const StockItem<Item> *Stock<Item>::Get(const Item *item) const
{
	auto it = find(this->begin(), this->end(), item);
	if(it == this->end())
		return nullptr;
	return &*it;
}
