/* ExclusiveItem.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef EXCLUSIVE_ITEM_H_
#define EXCLUSIVE_ITEM_H_



// An ExclusiveItem is an object stored as either a pointer or a stock
// object, but not both.
template <class Type>
class ExclusiveItem {
public:
	ExclusiveItem() = default;

	ExclusiveItem(const Type *item) : stockItem(item) {}
	explicit ExclusiveItem(Type &&item) : item(std::move(item)) {}

	ExclusiveItem(ExclusiveItem<Type> &&other) : stockItem(other.stockItem), item(std::move(other.item)) {}
	ExclusiveItem(const ExclusiveItem<Type> &other) : stockItem(other.stockItem), item(other.item) {}

	const Type *operator->() const noexcept { return stockItem ? stockItem : std::addressof(item); }
	const Type &operator*() const noexcept { return stockItem ? *stockItem : item; }

	ExclusiveItem<Type> &operator=(ExclusiveItem<Type> &&other) { stockItem = other.stockItem; item = std::move(other.item); return *this; }
	ExclusiveItem<Type> &operator=(const ExclusiveItem<Type> &other) { stockItem = other.stockItem; item = other.item; return *this; }
	bool operator==(const ExclusiveItem<Type> &other) const { return *this == *other; }
	bool operator!=(const ExclusiveItem<Type> &other) const { return !(*this == other); }


private:
	const Type *stockItem = nullptr;
	Type item;
};



#endif
