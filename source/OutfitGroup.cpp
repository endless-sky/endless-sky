/* OutfitGroup.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutfitGroup.h"

#include "Outfit.h"
#include "DataNode.h"
#include "GameData.h"

#include <cmath>



int64_t OutfitGroup::CostFunction(const Outfit *outfit, int age, double minValue, double lossPerDay)
{
	return static_cast<int64_t>(outfit->Cost() * std::max(minValue, 1. - (lossPerDay * age)));
}



void OutfitGroup::Clear()
{
	outfits.clear();
}



bool OutfitGroup::Empty() const
{
	return outfits.empty();
}



const std::map<int, int> *OutfitGroup::Find(const Outfit *outfit) const
{
	auto iter = outfits.find(outfit);
	if (iter != outfits.end())
		return &iter->second;
	return nullptr;
}



int64_t OutfitGroup::GetTotalCost() const
{
	// Get the total cost of every outfit. 
	int64_t cost = 0;
	for (auto it = begin(); it != end(); ++it)
		cost += it.GetTotalCost();
	return cost;
}



int64_t OutfitGroup::GetTotalCost(const Outfit *outfit) const
{
	int64_t cost = 0;
	for (auto it = find(outfit); it.GetOutfit() == outfit; ++it)
		cost += it.GetTotalCost();
	return cost;
}



int OutfitGroup::GetTotalCount(const Outfit *outfit) const
{
	int count = 0;
	auto matchingOutfits = outfits.find(outfit);
	if (matchingOutfits == outfits.end())
		return 0;
	for (auto it : matchingOutfits->second)
		count += it.second;
	return count;
}



int64_t OutfitGroup::GetCost(const Outfit* outfit, int count, bool oldestFirst) const
{
	int64_t cost = 0;
	auto matchingOutfits = outfits.find(outfit);
	if (matchingOutfits == outfits.end())
		return 0;
	if (oldestFirst)
	{
		auto it = matchingOutfits->second.rbegin();
		for (; it != matchingOutfits->second.rend(); ++it)
		{
			cost += CostFunction(outfit, it->first);	
			if (--count <= 0)
				break;
		}
	}
	else 
	{ // newest first
		auto it = matchingOutfits->second.begin();
		for (; it != matchingOutfits->second.end(); ++it)
		{
			cost += CostFunction(outfit, it->first);	
			if (--count <= 0)
				break;
		}		
	}
	return cost;
}



void OutfitGroup::AddOutfit(const Outfit* outfit, int count, int age)
{/*
	auto it = outfits.find(outfit);
	if(it == outfits.end())
		outfits[outfit] = count;
	else
	{
		it->second += count;
		if(!it->second)
			outfits.erase(it);
	}
*/}



void OutfitGroup::RemoveOutfit(const Outfit* outfit, int count, bool oldestFirst)
{
	
}



void OutfitGroup::TransferOutfits(const Outfit *outfit, int count, OutfitGroup* to, bool oldestFirst, int defaultAge)
{
	
}



void OutfitGroup::IncrementDate()
{
	
}



OutfitGroup::iterator::iterator (const OutfitGroup* group, bool begin)
{
	myGroup = group;
	if (begin)
	{
		outerIter = myGroup->outfits.begin();
		innerIter = outerIter->second.begin();
	}
	else
	{
		outerIter = myGroup->outfits.end();
		innerIter = outerIter->second.end();
	}
}



OutfitGroup::iterator::iterator (const OutfitGroup* group, const Outfit* outfit) : myGroup(group)
{
	outerIter = myGroup->outfits.find(outfit);
	innerIter = outerIter->second.begin();
}



// Operators that form allow use with a range-based for loop
bool OutfitGroup::iterator::operator!= (const OutfitGroup::iterator& other) const
{
	return outerIter != other.outerIter || innerIter != other.innerIter;
}



bool OutfitGroup::iterator::operator== (const OutfitGroup::iterator& other) const
{
	return outerIter == other.outerIter && innerIter == other.innerIter;
}



OutfitGroup::iterator OutfitGroup::iterator::operator* () const
{
	return *this;
}



const OutfitGroup::iterator& OutfitGroup::iterator::operator++ ()
{
	
	if (innerIter == outerIter->second.end())
	{
		outerIter++;
		innerIter = outerIter->second.begin();
	}
	else
		innerIter++;
	
	return *this;
}



const Outfit* OutfitGroup::iterator::GetOutfit() const
{
	return outerIter->first;
}



int OutfitGroup::iterator::GetAge() const
{
	return innerIter->first;
}



int OutfitGroup::iterator::GetQuantity() const
{
	return innerIter->second;
}



int64_t OutfitGroup::iterator::GetTotalCost() const
{
	return OutfitGroup::CostFunction(GetOutfit(), GetAge()) * GetQuantity();
}



int64_t OutfitGroup::iterator::GetCostPerOutfit() const
{
	return CostFunction(GetOutfit(), GetAge());
}



// OutfitGroup functions that return iterators.
OutfitGroup::iterator OutfitGroup::begin() const
{
	return iterator(this, true);
}



OutfitGroup::iterator OutfitGroup::end() const
{
	return iterator(this, false);
}



OutfitGroup::iterator OutfitGroup::find(const Outfit *outfit) const
{
	return iterator(this, outfit);
}
