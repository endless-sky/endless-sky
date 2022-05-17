/* UnionItem.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef UNION_ITEM_H_
#define UNION_ITEM_H_



// A UnionItem is an object stored as either a pointer or a stock
// object, but not both.
template <class Type>
class UnionItem {
public:
	UnionItem() = default;
	UnionItem(Type &&item) : item(item) {}
	UnionItem(const Type *item) : stockItem(item) {}
	const Type &GetItem() const noexcept { return stockItem ? *stockItem : item; }
	bool operator==(UnionItem<Type> other) const { return GetItem() == other.GetItem(); }
	bool operator!=(UnionItem<Type> other) const { return !(*this == other); }


private:
	Type item;
	const Type *stockItem = nullptr;
};



#endif
