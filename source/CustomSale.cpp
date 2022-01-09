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
	const std::map<CustomSale::SellType, const std::string> show{{CustomSale::SellType::NONE, ""}, {CustomSale::SellType::VISIBLE, ""},
		{CustomSale::SellType::IMPORT, "import"}, {CustomSale::SellType::HIDDEN, "hidden"}};
	const double DEFAULT = -100000.;
}



void CustomSale::Load(const DataNode &node, const Set<Sale<Outfit>> &items, const Set<Outfit> &outfits)
{
	for(const DataNode &child : node)
	{
		const std::string &token = child.Token(0);
		if((token == "clear" || token == "remove"))
		{
			if(child.Size() == 1)
				clear();
			else if(child.Token(1) == "outfit" && child.Size() >= 3)
			{
				const Outfit *outfit = outfits.Get(child.Token(2));
				relativeOutfitPrices.erase(outfit);
				relativeOutfitOffsets.erase(outfit);
			}
			else if(child.Token(1) == "outfitter" && child.Size() >= 3)
			{
				const Sale<Outfit> *item = items.Get(child.Token(2));
				relativePrices.erase(item);
				relativeOffsets.erase(item);
			}
		}
		else if(token == "outfit")
		{
			for(const DataNode &grandChild : child)
			{
				bool value = (grandChild.Token(0) == "value");
				bool offset = (grandChild.Token(0) == "offset");
				if(value || offset)
					for(const DataNode &kid : grandChild)
					{
						bool add = (kid.Token(0) == "add");
						const Outfit *outfit = outfits.Get(kid.Token(add));
						
						if(kid.Size() < 2 + add)
							continue;
						if(value)
						{
							if(add)
								relativeOutfitPrices[outfit] += kid.Value(2);
							else
								relativeOutfitPrices[outfit] = kid.Value(1);
							if(child.Size() == 2 + add)
								relativeOutfitPrices[outfit] /= outfit->Cost();
						}
						else if(offset)
						{
							if(add)
								relativeOutfitOffsets[outfit] += kid.Value(2);
							else
								relativeOutfitOffsets[outfit] = kid.Value(1);
							if(child.Size() == 2 + add)
								relativeOutfitOffsets[outfit] /= outfit->Cost();
						}
					}
				else if(grandChild.Size() >= 2)
				{
					const Outfit *outfit = outfits.Get(grandChild.Token(0));
					relativeOutfitPrices[outfit] = grandChild.Value(1) / outfit->Cost();
				}
			}
		}
		else if(token == "outfitter")
		{
			for(const DataNode &grandChild : child)
			{
				bool value = (grandChild.Token(0) == "value");
				bool offset = (grandChild.Token(0) == "offset");
				if(value || offset)
					for(const DataNode &kid : grandChild)
					{
						bool add = (kid.Token(0) == "add");
						const Sale<Outfit> *item = items.Get(kid.Token(add));

						if(kid.Size() < 2 + add)
							continue;
						if(value)
						{
							if(add)
								relativePrices[item] += kid.Value(2);
							else
								relativePrices[item] = kid.Value(1);
						}
						else if(offset)
						{
							if(add)
								relativeOffsets[item] += kid.Value(2);
							else
								relativeOffsets[item] = kid.Value(1);
						}
					}
				else if(grandChild.Size() >= 2)
				{
					const Sale<Outfit> *item = items.Get(grandChild.Token(0));
					relativePrices[item] = grandChild.Value(1);
				}
			}
		}
		else if(token == "hidden" || token == "import")
		{
			for(const auto& it : show)
				if(token == it.second)
					sellType = it.first;
		}
		else if(token == "visible")
			sellType = CustomSale::SellType::VISIBLE;
		else if(token == "location")
		{
			if(child.Size() == 1)
				locationFilter.Load(child);
			else if(child.Size() == 2)
			{
				source = GameData::Planets().Get(child.Token(1));
				if(child.HasChildren())
					child.PrintTrace("Warning: location filter ignored due to use of explicit planet:");
			}
			else
				child.PrintTrace("Warning: use a location filter to choose from multiple planets:");
		}
		else if(token == "conditions")
			toApply.Load(child);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	if(sellType == SellType::NONE)
		sellType = SellType::VISIBLE;
}



// Can only add between CustomSales of same sellType.
bool CustomSale::Add(const CustomSale &other)
{
	// sellType::NONE means a new CustomSale created with no data and default sellType
	if(this->sellType == SellType::NONE)
		this->sellType = other.sellType;
	else if(other.sellType != this->sellType)
		return false;
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
		else
			item->second += it.second;
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
		else
			item->second += it.second;
	}
	return true;
}



double CustomSale::GetRelativeCost(const Outfit *item) const
{
	const auto& baseRelative = relativeOutfitPrices.find(item);
	double baseRelativePrice = (baseRelative != relativeOutfitPrices.cend() ? baseRelative->second : DEFAULT);
	if(baseRelativePrice == DEFAULT)
		for(const auto& it : relativePrices)
			if(it.first->Has(item))
			{
				baseRelativePrice = it.second;
				break;
			}
	const auto& baseOffset = relativeOutfitOffsets.find(item);
	double baseOffsetPrice = (baseOffset != relativeOutfitOffsets.cend() ? baseOffset->second : DEFAULT);
	for(const auto& it : relativeOffsets)
		if(it.first->Has(item))
		{
			if(baseOffsetPrice == DEFAULT)
				baseOffsetPrice = 0.;
			baseOffsetPrice += it.second;
		}
	if(baseRelativePrice != DEFAULT)
		return baseRelativePrice + (baseOffsetPrice != DEFAULT ? baseRelativePrice * baseOffsetPrice : 0.);
	else if(baseOffsetPrice != DEFAULT)
		return 1. + baseOffsetPrice;
	else
		return -1.;
}



CustomSale::SellType CustomSale::GetSellType() const
{
	return sellType;
}



const std::string &CustomSale::GetShown(CustomSale::SellType sellType)
{
	return show.find(sellType)->second;
}



const Sale<Outfit> &CustomSale::GetOutfits() const
{
	seen.clear();
	for(auto it : relativeOutfitPrices)
		seen.insert(it.first);
	for(auto it : relativeOutfitOffsets)
		seen.insert(it.first);	
	for(auto sale : relativePrices)
		seen.Add(*sale.first);
	for(auto sale : relativeOffsets)
		seen.Add(*sale.first);	
	return seen;
}



bool CustomSale::Has(const Outfit *item) const
{
	return GetRelativeCost(item) != -1.;
}



bool CustomSale::Matches(const Planet *planet, const ConditionSet::Conditions &conditions) const
{
	return ((source != nullptr) ? source == planet : locationFilter.Matches(planet)) && 
		(toApply.IsEmpty() || toApply.Test(conditions));
}



void CustomSale::clear()
{
	sellType = SellType::NONE;
	relativeOffsets.clear();
	relativePrices.clear();
	relativeOutfitOffsets.clear();
	relativeOutfitPrices.clear();
}
