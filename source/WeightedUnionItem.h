/* WeightedUnionItem.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WEIGHTED_UNION_ITEM_H_
#define WEIGHTED_UNION_ITEM_H_



// A WeightedUnionItem is a weighted object stored as either a pointer or a stock
// object, but not both.
template <class Type>
class WeightedUnionItem {
public:
	WeightedUnionItem() = default;
	WeightedUnionItem(Type &&item, int weight) : item(item), weight(weight) {}
	WeightedUnionItem(const Type *item, int weight) : stockItem(item), weight(weight) {}
	const Type &GetItem() const noexcept { return stockItem ? *stockItem : item; }
	int Weight() const noexcept { return weight; }
	bool operator==(WeightedUnionItem<Type> other) const { return GetItem() == other.GetItem(); }
	bool operator!=(WeightedUnionItem<Type> other) const { return !(*this == other); }


private:
	Type item;
	const Type *stockItem = nullptr;
	int weight = 0;
};



#endif
