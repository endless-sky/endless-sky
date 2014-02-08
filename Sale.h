/* Sale.h
Michael Zahniser, 8 Feb 2014

Class representing a set of items that are for sale on a given planet.
*/

#ifndef SALE_H_INCLUDED
#define SALE_H_INCLUDED

#include "DataFile.h"
#include "Set.h"

#include <algorithm>
#include <set>
#include <vector>



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
		insert(items.Get(child.Token(0)));
}



template <class Item>
void Sale<Item>::Add(const Sale<Item> &other)
{
	insert(other.begin(), other.end());
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
