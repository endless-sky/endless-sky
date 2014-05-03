/* Sale.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SALE_H_
#define SALE_H_

#include "DataFile.h"
#include "Set.h"

#include <algorithm>
#include <set>
#include <vector>



// Class representing a set of items that are for sale on a given planet.
template <class Item>
class Sale : public std::set<const Item *> {
public:
	void Load(const DataFile::Node &node, const Set<Item> &items);
	
	void Add(const Sale<Item> &other);
	
	void StoreList(std::vector<const Item *> *list) const;
};



template <class Item>
void Sale<Item>::Load(const DataFile::Node &node, const Set<Item> &items)
{
	for(const DataFile::Node &child : node)
		this->insert(items.Get(child.Token(0)));
}



template <class Item>
void Sale<Item>::Add(const Sale<Item> &other)
{
	this->insert(other.begin(), other.end());
}



template <class Item>
void Sale<Item>::StoreList(std::vector<const Item *> *list) const
{
	list->clear();
	for(const Item *item : *this)
		list->push_back(item);
	
	std::sort(list->begin(), list->end(),
		[](const Item *a, const Item *b)
		{
			return (a->Cost() > b->Cost());
		});
}



#endif
