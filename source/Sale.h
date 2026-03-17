/* Sale.h
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

#pragma once

#include "DataNode.h"
#include "Set.h"

#include <set>
#include <string>



// Class representing a set of items that are for sale on a given planet.
// Multiple sale sets can be merged together into a single one.
template<class Item>
class Sale : public std::set<const Item *> {
public:
	void Load(const DataNode &node, const Set<Item> &items, bool preventModifiers = false);
	void LoadSingle(const DataNode &child, const Set<Item> &items, bool preventModifiers = false);

	void Add(const Sale<Item> &other);

	bool Has(const Item *item) const;
};



template<class Item>
void Sale<Item>::Load(const DataNode &node, const Set<Item> &items, bool preventModifiers)
{
	for(const DataNode &child : node)
		LoadSingle(child, items, preventModifiers);
}



template<class Item>
void Sale<Item>::LoadSingle(const DataNode &child, const Set<Item> &items, bool preventModifiers)
{
	const std::string &token = child.Token(0);
	bool remove = (token == "clear" || token == "remove");
	bool add = (token == "add");
	if((remove || add) && preventModifiers)
	{
		child.PrintTrace("Cannot \"add\" or \"remove\" inside a \"stock\" node:");
		return;
	}
	bool hasValue = child.Size() >= 2;
	if(remove && !hasValue)
		this->clear();
	else if(remove && hasValue)
		this->erase(items.Get(child.Token(1)));
	else if(add && hasValue)
		this->insert(items.Get(child.Token(1)));
	else
		this->insert(items.Get(token));
}



template<class Item>
void Sale<Item>::Add(const Sale<Item> &other)
{
	this->insert(other.begin(), other.end());
}



template<class Item>
bool Sale<Item>::Has(const Item *item) const
{
	return this->count(item);
}
