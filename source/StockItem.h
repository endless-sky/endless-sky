/* StockItem.h
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

#include "ShopPricing.h"



// Class representing a particular item for sale in a shop.
// Includes the price modifiers for the item being sold. Multiple
// StockItems that represent the same item for sale can be combined,
// merging the price modifiers from both items.
template <class Item>
class StockItem {
public:
	StockItem(const Item *item, ShopPricing buyModifier, ShopPricing sellModifier);

	const Item *GetItem() const;

	const ShopPricing &BuyModifier() const;
	const ShopPricing &SellModifier() const;

	// Combine StockItems that contain the same item.
	void Combine(const StockItem<Item> &other);

	// Two StockItems are equal if they contain the same item and have the same precedence.
	bool operator==(const StockItem<Item> &other) const;
	bool operator==(const Item *item) const;
	// For storing in sorted containers.
	bool operator<(const StockItem<Item> &other) const;


private:
	const Item *item = nullptr;

	ShopPricing buyModifier;
	ShopPricing sellModifier;
};



template <class Item>
StockItem<Item>::StockItem(const Item *item, ShopPricing buyModifier, ShopPricing sellModifier)
	: item(item), buyModifier(buyModifier), sellModifier(sellModifier)
{
}



template <class Item>
const Item *StockItem<Item>::GetItem() const
{
	return item;
}



template <class Item>
const ShopPricing &StockItem<Item>::BuyModifier() const
{
	return buyModifier;
}



template <class Item>
const ShopPricing &StockItem<Item>::SellModifier() const
{
	return sellModifier;
}



template <class Item>
void StockItem<Item>::Combine(const StockItem<Item> &other)
{
	this->buyModifier.Combine(other.buyModifier);
	this->sellModifier.Combine(other.sellModifier);
}



template <class Item>
bool StockItem<Item>::operator==(const StockItem<Item> &other) const
{
	return other.item == item;
}



template <class Item>
bool StockItem<Item>::operator==(const Item *item) const
{
	return this->item == item;
}



template <class Item>
bool StockItem<Item>::operator<(const StockItem<Item> &other) const
{
	return this->item < item;
}
