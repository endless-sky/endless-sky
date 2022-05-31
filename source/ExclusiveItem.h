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

#include <memory>
#include <utility>



// An ExclusiveItem is an object stored as either a pointer or a stock
// object, but not both.
template <class Type>
class ExclusiveItem {
public:
	ExclusiveItem() = default;

	ExclusiveItem(const Type *item) : stockItem(item) {}
	explicit ExclusiveItem(Type &&item) : item(std::move(item)) {}

	ExclusiveItem(ExclusiveItem&&) = default;
	ExclusiveItem &operator=(ExclusiveItem&&) = default;
	ExclusiveItem(const ExclusiveItem&) = default;
	ExclusiveItem &operator=(const ExclusiveItem&) = default;

	const Type *operator->() const noexcept { return stockItem ? stockItem : std::addressof(item); }
	const Type &operator*() const noexcept { return stockItem ? *stockItem : item; }

	bool operator==(const ExclusiveItem &other) const { return this->operator*() == other.operator*(); }
	bool operator!=(const ExclusiveItem &other) const { return !(this->operator*() == other.operator*()); }


private:
	const Type *stockItem = nullptr;
	Type item;
};



#endif
