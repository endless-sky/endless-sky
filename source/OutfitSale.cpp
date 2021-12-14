/* OutfitSale.cpp
Copyright (c) 2021 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DataNode.h"
#include "OutfitSale.h"
#include "Set.h"
#include "Sold.h"

#include <string>



void OutfitSale::Load(const DataNode &node, const Set<Outfit> &items)
{
	for(const DataNode &child : node)
	{
		const std::string &token = child.Token(0);
		bool remove = (token == "clear" || token == "remove");
		if(remove && child.Size() == 1)
			soldOutfits.clear();
		else if(remove && child.Size() >= 2)
			soldOutfits.erase(items.Get(child.Token(1)));
		else if(token == "add" && child.Size() >= 2)
			soldOutfits[items.Get(child.Token(1))].SetBase(child.Size() > 2 ? 
				child.Value(2) : 0., child.Size() > 3 ? Sold::StringToSellType(child.Token(3)) : Sold::SellType::VISIBLE);
		else if(token == "hidden" || token == "import")
			for(const DataNode &subChild : child)
				soldOutfits[items.Get(subChild.Token(0))].SetBase(subChild.Size() > 1 ? 
					subChild.Value(1) : 0., Sold::StringToSellType(token));
		else
			soldOutfits[items.Get(child.Token(0))].SetBase(child.Size() > 1 ? 
				child.Value(1) : 0., child.Size() > 2 ? Sold::StringToSellType(child.Token(2)) : Sold::SellType::VISIBLE);
	}
}



// operator[] is used to override existing data instead, priorities are
// hidden > import > highest price
void OutfitSale::Add(const OutfitSale &other)
{
	for(const auto& it : other.GetSoldOutfits())
	{
		const Sold* sold = GetSold(it.first);
		if(!sold)
		{
			// This will not override existing items.
			soldOutfits.insert(it);
			continue;
		}

		if(sold->GetSellType() == it.second.GetSellType())
			soldOutfits[it.first].SetCost(std::max(sold->GetCost(), it.second.GetCost()));
		else if(sold->GetSellType() < it.second.GetSellType())
			soldOutfits[it.first].SetBase(it.second.GetCost(), Sold::StringToSellType(it.second.GetShown()));
	}
}



const Sold* OutfitSale::GetSold(const Outfit *item) const
{
	auto sold = soldOutfits.find(item);
	return (sold != soldOutfits.end()) ? &sold->second : nullptr;
}




double OutfitSale::GetCost(const Outfit *item) const
{
	const Sold* sold = GetSold(item);
	return sold ? sold->GetCost() : 0.;
}




Sold::SellType OutfitSale::GetShown(const Outfit *item) const
{
	const Sold* sold = GetSold(item);
	return sold ? sold->GetSellType() : Sold::SellType::NONE;
}




bool OutfitSale::Has(const Outfit *item) const
{
	return soldOutfits.count(item);
}



const std::map<const Outfit *, Sold> &OutfitSale::GetSoldOutfits() const
{
	return soldOutfits;
}



void OutfitSale::clear()
{
	soldOutfits.clear();
}