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

using namespace std;

namespace
{
	const auto show = map<CustomSale::SellType, const string> {
		{CustomSale::SellType::NONE, ""},
		{CustomSale::SellType::VISIBLE, ""},
		{CustomSale::SellType::IMPORT, "import"},
		{CustomSale::SellType::HIDDEN, "hidden"},
	};
	const double DEFAULT = -100000.;
}



void CustomSale::Load(const DataNode &node, const Set<Sale<Outfit>> &items, const Set<Outfit> &outfits, const string &mode)
{
	for(const DataNode &child : node)
	{
		const string &token = child.Token(0);
		bool isValue = (child.Token(0) == "value");
		bool isOffset = (child.Token(0) == "offset");
		if((token == "clear" || token == "remove"))
		{
			if(child.Size() == 1)
				Clear();
			else if(child.Token(1) == "outfit")
			{
				if(child.Size() >= 3)
				{
					const Outfit *outfit = outfits.Get(child.Token(2));
					relativeOutfitPrices.erase(outfit);
					relativeOutfitOffsets.erase(outfit);
				}
				else
				{
					relativeOutfitOffsets.clear();
					relativeOutfitPrices.clear();
				}
			}
			else if(child.Token(1) == "outfitter")
			{
				if(child.Size() >= 3)
				{
					const Sale<Outfit> *outfitter = items.Get(child.Token(2));
					relativePrices.erase(outfitter);
					relativeOffsets.erase(outfitter);
				}
				else
				{
					relativeOffsets.clear();
					relativePrices.clear();
				}
			}
			else if(child.Token(1) == "location")
				locationFilter = LocationFilter{};
			else if(child.Token(1) == "conditions")
				conditions = ConditionSet{};
			else
				child.PrintTrace("Skipping unrecognized clearing/deleting:");
		}
		else if(token == "add")
		{
			if(child.Token(1) == "location" && child.Size() == 1)
				locationFilter.Load(child);
			else if(child.Token(1) == "conditions")
				conditions.Load(child);
			else
				child.PrintTrace("Skipping unrecognized add:");
		}
		else if(token == "hidden")
			sellType = SellType::HIDDEN;
		else if(token == "import")
			sellType = SellType::IMPORT;
		else if(token == "visible")
			sellType = SellType::VISIBLE;
		else if(token == "location")
		{
			if(child.Size() == 1)
			{
				location = nullptr;
				locationFilter = LocationFilter{};
				locationFilter.Load(child);
			}
			else if(child.Size() == 2)
			{
				location = GameData::Planets().Get(child.Token(1));
				locationFilter = LocationFilter{};
				if(child.HasChildren())
					child.PrintTrace("Warning: location filter ignored due to use of explicit planet:");
			}
			else
				child.PrintTrace("Warning: use a location filter to choose from multiple planets:");
		}
		else if(token == "conditions")
		{
			conditions = ConditionSet{};
			conditions.Load(child);
		}
		else if(mode == "outfits")
		{
			bool isAdd = false;
			const Outfit *outfit = nullptr;
			auto parseValueOrOffset = [&isAdd, &outfit](double &amount, const DataNode &line) {
				if(isAdd)
					amount += line.Size() > 2 ? line.Value(2) : 1.;
				else
					amount = line.Size() > 1 ? line.Value(1) : 1;
				// If there is a third element it means a relative % and not a raw value is specified.
				if(line.Size() == 2 + isAdd)
					amount /= outfit->Cost();
			};
			if(isValue || isOffset)
				for(const DataNode &kid : child)
				{
					isAdd = (kid.Token(0) == "add");
					outfit = outfits.Get(kid.Token(isAdd));

					if(kid.Size() < 1 + isAdd)
						continue;

					if(isValue)
						parseValueOrOffset(relativeOutfitPrices[outfit], kid);
					else if(isOffset)
						parseValueOrOffset(relativeOutfitOffsets[outfit], kid);
				}
			// Default behavior assumes value.
			else if(child.Size() >= 1)
			{
				isAdd = (child.Token(0) == "add");
				outfit = outfits.Get(child.Token(isAdd));
				parseValueOrOffset(relativeOutfitPrices[outfit], child);
			}
			else
				child.PrintTrace("Skipping unrecognized (outfit assumed) attribute:");
		}
		else if(mode == "outfitters")
		{
			if(isValue || isOffset)
				for(const DataNode &kid : child)
				{
					bool isAdd = (kid.Token(0) == "add");
					const Sale<Outfit> *outfitter = items.Get(kid.Token(isAdd));

					if(kid.Size() < 1 + isAdd)
						continue;

					auto parseValueOrOffset = [isAdd, outfitter](double &amount, const DataNode &line) {
						// Only % may be specified using outfitter modification.
						if(isAdd)
							amount += line.Size() > 1 ? line.Value(2) : 1.;
						else
							amount = line.Size() > 1 ? line.Value(1) : 1;
					};

					if(isValue)
						parseValueOrOffset(relativePrices[outfitter], kid);
					else if(isOffset)
						parseValueOrOffset(relativeOffsets[outfitter], kid);
				}
			// Default behavior assumes value.
			else if(child.Size() >= 2)
			{
				const Sale<Outfit> *outfitter = items.Get(child.Token(0));
				relativePrices[outfitter] = child.Value(1);
			}
			else
				child.PrintTrace("Skipping unrecognized (outfitter assumed) attribute:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	CheckIsEmpty();
}



bool CustomSale::Add(const CustomSale &other)
{
	// sellType::NONE means a new CustomSale created with no data and default sellType
	if(this->sellType == SellType::NONE)
		this->sellType = other.sellType;
	else if(other.sellType != this->sellType)
		return false;

	for(const auto &it : other.relativePrices)
	{
		auto ours = relativePrices.find(it.first);
		if(ours == relativePrices.end())
			relativePrices.emplace(it.first, it.second);
		else if(ours->second < it.second)
			ours->second = it.second;
	}
	for(const auto &it : other.relativeOffsets)
	{
		auto ours = relativeOffsets.find(it.first);
		if(ours == relativeOffsets.end())
			relativeOffsets.emplace(it.first, it.second);
		else
			ours->second += it.second;
	}
	for(const auto &it : other.relativeOutfitPrices)
	{
		auto ours = relativeOutfitPrices.find(it.first);
		if(ours == relativeOutfitPrices.end())
			relativeOutfitPrices.emplace(it.first, it.second);
		else if(ours->second < it.second)
			ours->second = it.second;
	}
	for(const auto &it : other.relativeOutfitOffsets)
	{
		auto ours = relativeOutfitOffsets.find(it.first);
		if(ours == relativeOutfitOffsets.end())
			relativeOutfitOffsets.emplace(it.first, it.second);
		else
			ours->second += it.second;
	}
	return true;
}



double CustomSale::GetRelativeCost(const Outfit &item) const
{
	// First look if it is in the outfits, then in the outfitters.
	const auto& baseRelative = relativeOutfitPrices.find(&item);
	double baseRelativePrice = (baseRelative != relativeOutfitPrices.cend() ? baseRelative->second : DEFAULT);
	if(baseRelativePrice == DEFAULT)
		for(const auto& it : relativePrices)
			if(it.first->Has(&item))
			{
				baseRelativePrice = it.second;
				break;
			}
	const auto& baseOffset = relativeOutfitOffsets.find(&item);
	double baseOffsetPrice = (baseOffset != relativeOutfitOffsets.cend() ? baseOffset->second : DEFAULT);
	for(const auto& it : relativeOffsets)
		if(it.first->Has(&item))
		{
			if(baseOffsetPrice == DEFAULT)
				baseOffsetPrice = 0.;
			baseOffsetPrice += it.second;
		}
	// Apply the relative offset on top of the relative price.
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



const string &CustomSale::GetShown(CustomSale::SellType sellType)
{
	return show.find(sellType)->second;
}



const Sale<Outfit> CustomSale::GetOutfits() const
{
	Sale<Outfit> seen;
	for(auto it : relativeOutfitPrices)
		seen.insert(it.first);
	for(auto it : relativeOutfitOffsets)
		seen.insert(it.first);
	for(auto &&sale : relativePrices)
		seen.Add(*sale.first);
	for(auto &&sale : relativeOffsets)
		seen.Add(*sale.first);
	return seen;
}



bool CustomSale::Has(const Outfit &item) const
{
	return GetRelativeCost(item) != -1.;
}



bool CustomSale::Matches(const Planet &planet, const std::map<std::string, int64_t> &playerConditions) const
{
	return (location ? location == &planet : locationFilter.Matches(&planet)) &&
		(conditions.IsEmpty() || conditions.Test(playerConditions));
}



void CustomSale::Clear()
{
	*this = CustomSale{};
}



void CustomSale::CheckIsEmpty()
{
	if(relativeOffsets.empty() && relativePrices.empty() &&
		relativeOutfitOffsets.empty() && relativeOutfitPrices.empty())
		sellType = SellType::NONE;
	else if(sellType == SellType::NONE)
		sellType = SellType::VISIBLE;
}
