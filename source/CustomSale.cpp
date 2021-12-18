/* CustomSale.cpp
Copyright (c) 2021 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CustomSale.h"
#include "DataNode.h"
#include "GameData.h"
#include "Planet.h"
#include "Set.h"

#include <string>

namespace 
{
	const std::map<CustomSale::SellType, const std::string> show{{CustomSale::SellType::VISIBLE, ""},
		{CustomSale::SellType::IMPORT, "import"}, {CustomSale::SellType::HIDDEN, "hidden"}, {CustomSale::SellType::NONE, ""}};
}



// take care of the "add" even though idk why that keyword exists
void CustomSale::Load(const DataNode &node, const Set<Sale<Outfit>> &items, const Set<Outfit> &outfits)
{
	for(const DataNode &child : node)
	{
		const std::string &token = child.Token(0);
		bool remove = (token == "clear" || token == "remove");
		if(remove && child.Size() == 1)
		{
			clear();
		}
		else if(remove && child.Token(1) == "outfitter" && child.Size() >= 3)
		{
			const Sale<Outfit> *item = items.Get(child.Token(2));
			relativePrices.erase(item);
			relativeOffsets.erase(item);
		}
		else if(remove && child.Token(1) == "outfit" && child.Size() >= 3)
		{
			const Outfit *outfit = outfits.Get(child.Token(2));
			relativeOutfitPrices.erase(outfit);
			relativeOutfitOffsets.erase(outfit);
		}
		else if(token == "outfit" && child.Size() >= 3)
		{
			const Outfit *outfit = outfits.Get(child.Token(1));
			if(child.Token(2) == "value")
			{
				relativeOutfitPrices[outfit] = child.Value(3);
				if(child.Size() < 5)
					relativeOutfitPrices[outfit] /= outfit->Cost();
			}
			else if(child.Token(2) == "offset")
			{
				relativeOutfitOffsets[outfit] = child.Value(3);
				if(child.Size() < 5)
					relativeOutfitOffsets[outfit] /= outfit->Cost();
			}
		}
		else if(token == "outfitter" && child.Size() >= 2)
		{
			const Sale<Outfit> *item = items.Get(child.Token(1));
			if(child.Token(2) == "value")
				relativePrices[item] = child.Value(3);
			else if(child.Token(2) == "offset")
				relativeOffsets[item] = child.Value(3);
		}
		else if(token == "hidden" || token == "import")
		{
			for(const auto& it : show)
				if(token == it.second)
					shown = it.first;
		}
		else if(child.Token(0) == "location")
			locationFilter.Load(child);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// priorities are hidden > import > highest price
void CustomSale::Add(const CustomSale &other)
{
	if(other.shown > this->shown)
		clear();
	for(const auto& it : other.relativePrices)
	{
		const auto& item = relativePrices.find(it.first);
		if(item == relativePrices.cend())
			relativePrices[it.first] = it.second;
		else if(item->second < it.second)
			item->second = it.second;
	}
	for(const auto& it : other.relativeOffsets)
	{
		const auto& item = relativeOffsets.find(it.first);
		if(item == relativeOffsets.cend())
			relativeOffsets[it.first] = it.second;
		else if(item->second < it.second)
			item->second = it.second;
	}
	for(const auto& it : other.relativeOutfitPrices)
	{
		const auto& item = relativeOutfitPrices.find(it.first);
		if(item == relativeOutfitPrices.cend())
			relativeOutfitPrices[it.first] = it.second;
		else if(item->second < it.second)
			item->second = it.second;
	}
	for(const auto& it : other.relativeOutfitOffsets)
	{
		const auto& item = relativeOutfitOffsets.find(it.first);
		if(item == relativeOutfitOffsets.cend())
			relativeOutfitOffsets[it.first] = it.second;
		else if(item->second < it.second)
			item->second = it.second;
	}
}



double CustomSale::GetRelativeCost(const Outfit *item) const
{
	const auto& baseRelative = relativeOutfitPrices.find(item);
	const auto& baseOffset = relativeOutfitOffsets.find(item);
	double finalPrice = (baseRelative != relativeOutfitPrices.cend() ? baseRelative->second : 1.) +
						(baseOffset != relativeOutfitOffsets.cend() ? baseOffset->second : 0.);
	if(finalPrice != 1.)
		return finalPrice;
	else
	{
		double baseRelative = -1.;
		for(const auto& it : relativePrices)
			if(it.first->Has(item))
			{
				baseRelative = it.second;
				break;
			}
		double baseOffset = 0.;
		for(const auto& it : relativeOffsets)
			if(it.first->Has(item))
			{
				baseOffset = it.second;
				break;
			}
		return baseRelative + baseOffset;
	}
}



CustomSale::SellType CustomSale::GetSellType() const
{
	return shown;
}



const std::string &CustomSale::GetShown(CustomSale::SellType sellType)
{
	return show.find(sellType)->second;
}



bool CustomSale::Has(const Outfit *item) const
{
	return GetRelativeCost(item) != -1.;
}



bool CustomSale::HasPlanet(const Planet *planet) const
{
	return locationFilter.Matches(planet);
}



void CustomSale::clear()
{
	relativeOffsets.clear();
	relativePrices.clear();
	relativeOutfitOffsets.clear();
	relativeOutfitPrices.clear();
}