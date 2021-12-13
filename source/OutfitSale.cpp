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
			this->clear();
		else if(remove && child.Size() >= 2)
			this->erase(items.Get(child.Token(1)));
		else if(token == "add" && child.Size() >= 2)
			(*this)[items.Get(child.Token(1))].SetBase(child.Size() > 2 ? 
				child.Value(2) : 0., child.Size() > 3 ? child.Token(3) : "");
		else if(token == "hidden" || token == "import")
			for(const DataNode &subChild : child)
				(*this)[items.Get(subChild.Token(0))].SetBase(subChild.Size() > 1 ? 
					subChild.Value(1) : 0., token);
		else
			(*this)[items.Get(child.Token(0))].SetBase(child.Size() > 1 ? 
				child.Value(1) : 0., child.Size() > 2 ? child.Token(2) : "");
	}
}



// operator[] is used to override existing data instead, priorities are
// hidden > import > highest price
void OutfitSale::Add(const OutfitSale &other)
{
	for(const auto& it : other)
	{
		const Sold* sold = GetSold(it.first);
		if(!sold)
		{
			// This will not override existing items.
			this->insert(it);
			continue;
		}

		if(sold->GetShown() == it.second.GetShown())
			(*this)[it.first].SetCost(std::max(sold->GetCost(), it.second.GetCost()));
		else if(sold->GetShown() < it.second.GetShown())
			(*this)[it.first].SetBase(it.second.GetCost(), it.second.GetShow());
	}
}



const Sold* OutfitSale::GetSold(const Outfit *item) const
{
	auto sold = this->find(item);
	return (sold != this->end()) ? &sold->second : nullptr;
}




double OutfitSale::GetCost(const Outfit *item) const
{
	const Sold* sold = GetSold(item);
	return sold ? sold->GetCost() : 0.;
}




Sold::ShowSold OutfitSale::GetShown(const Outfit *item) const
{
	const Sold* sold = GetSold(item);
	return sold ? sold->GetShown() : Sold::ShowSold::NONE;
}




bool OutfitSale::Has(const Outfit *item) const
{
	return this->count(item);
}

