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
#include "Format.h"
#include "GameData.h"
#include "Random.h"

#include <cmath>
#include <vector>


int64_t OutfitGroup::CostFunction(const Outfit *outfit, int age, double minValue, double lossPerDay)
{
	if (outfit->Get("ageless") || outfit->Category() == "Ammunition")
		return outfit->Cost();
	return static_cast<int64_t>(outfit->Cost() * std::max(minValue, 1. - (lossPerDay * age)));
}



double OutfitGroup::CostFunction(int age, double minValue, double lossPerDay)
{
	return std::max(minValue, 1. - (lossPerDay * age));
}



int OutfitGroup::UsedAge(double minValue, double lossPerDay)
{	// Random between 30% and 90% depreciated.
	double fullDepreciationAge = (1. - minValue) / lossPerDay;
	int min = static_cast<int>(fullDepreciationAge * 0.2);
	int max = static_cast<int>(fullDepreciationAge * 0.7);
	return Random::Int(max-min) + min;
}



int OutfitGroup::PlunderAge(double minValue, double lossPerDay)
{	// Random between 90% and 100% depreciated.
	double fullDepreciationAge = (1. - minValue) / lossPerDay;
	int min = static_cast<int>(fullDepreciationAge * 0.8);
	int max = static_cast<int>(fullDepreciationAge);
	return Random::Int(max-min) + min;
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



double OutfitGroup::GetTotalAttribute(std::string attribute) const
{
	// Get the total value of an attribute (such as mass) over every outfit. 
	double value = 0;
	for (auto it = begin(); it != end(); ++it)
		value += it.GetOutfit()->Get(attribute) * it.GetQuantity();
	return value;
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
		return 0; // Don't have any.
	if (oldestFirst)
	{
		auto it = matchingOutfits->second.rbegin();
		for (; it != matchingOutfits->second.rend(); ++it)
		{
			int matched = std::min(it->second, count);
			cost += CostFunction(outfit, it->first) * matched;	
			count -= matched;
			if (count <= 0)
				break;
		}
	}
	else 
	{ // newest first
		auto it = matchingOutfits->second.begin();
		for (; it != matchingOutfits->second.end(); ++it)
		{
			int matched = std::min(it->second, count);
			cost += CostFunction(outfit, it->first) * matched;	
			count -= matched;
			if (count <= 0)
				break;
		}		
	}
	return cost;
}



// Can be used to remove outfits, but will only remove outfits of the specified age.
int OutfitGroup::AddOutfit(const Outfit* outfit, int count, int age)
{
	auto oit = outfits.find(outfit);
	if(oit == outfits.end()) 
	{
		outfits[outfit] = InnerMap();
		outfits[outfit][age] = count;
	}
	else
	{
		auto iit = oit->second.find(age);
		if (iit == oit->second.end())
			oit->second[age] = count;
		else 
		{
			oit->second[age] += count;
			if(!oit->second[age])
				oit->second.erase(iit);
		}
		if (oit->second.empty())
			outfits.erase(oit);
	}
	return count;
}



// Remove outfits of a given type, either oldest or newest first.
// Used for making transfers as well.  
int OutfitGroup::RemoveOutfit(const Outfit* outfit, int count, bool oldestFirst, OutfitGroup* to)
{
	auto oit = outfits.find(outfit);
	if(oit == outfits.end()) 
		return 0;

	int removed = 0;
	if (oldestFirst)
	{
		auto iit = oit->second.end();
		std::vector<InnerMap::iterator> toErase;
		do {
			--iit;
			int age = iit->first;
			int toRemove = std::min(iit->second, count - removed);
			removed += toRemove;
			oit->second[age] -= toRemove;
			if (to)
				to->AddOutfit(outfit, toRemove, age);
			if (!oit->second[age]) 
			{	
				toErase.push_back(iit);
			}
		} while(count > removed && iit != oit->second.begin());
		
		for (auto it : toErase)
			oit->second.erase(it);
	}
	else 
	{
		auto iit = oit->second.begin();
		while(count > removed && iit != oit->second.end()) 
		{
			int age = iit->first;
			int toRemove = std::min(iit->second, count - removed);
			removed += toRemove;
			oit->second[age] -= toRemove;
			if (to)
				to->AddOutfit(outfit, toRemove, age);
			if (!oit->second[age])
				oit->second.erase(iit++); // erase current and iterate.
			else 
				++iit; // just iterate.
		}
	}
	if (oit->second.empty())
		outfits.erase(oit);
	
	return removed;
}



// Needs to support all kinds of operations either on a group or between groups.
int OutfitGroup::TransferOutfits(const Outfit *outfit, int count, OutfitGroup* to, bool oldestFirst, int defaultAge)
{
	// Invalid inputs.
	if(!count || !outfit)
		return 0;
	// Use add/remove if there's no destination.
	if (!to)
	{
		if (count > 0)
			return RemoveOutfit(outfit, count, oldestFirst); // Transfer to nowhere = remove.
		else 
			return -AddOutfit(outfit, -count, defaultAge); // Transfer from nowhere = add.
	}
	// If count is negative but *to is valid, just turn the whole thing around.  
	if(count < 0)
		return -(to->TransferOutfits(outfit, -count, this, oldestFirst, defaultAge));
	// Transferring a positive number of outfits to a valid destination.
	// Use the remove function for this.
	return RemoveOutfit(outfit, count, oldestFirst, to);
}



// Go through the whole group and increment all the ages. 
void OutfitGroup::IncrementDate(int days)
{
	for (auto oit : outfits)
	{
		//make a copy
		InnerMap temp;
		temp.insert(oit.second.begin(), oit.second.end());
		// clear
		outfits[oit.first].clear();
		// re-insert with incremented age keys
		for (auto iit : temp)
			outfits[oit.first][iit.first + days] = iit.second;
	}
}



OutfitGroup::iterator::iterator (const OutfitGroup* group, bool begin)
{
	myGroup = group;	
	if (begin && !myGroup->outfits.empty())
	{
		outerIter = myGroup->outfits.begin();
		innerIter = outerIter->second.begin();
		isEnd = false;
	}
	else
	{
		outerIter = myGroup->outfits.end();
		innerIter = outerIter->second.begin();
		isEnd = true;
	}
}



OutfitGroup::iterator::iterator (const OutfitGroup* group, const Outfit* outfit)
{
	myGroup = group;
	isEnd = false;
	outerIter = myGroup->outfits.find(outfit);
	if (outerIter == myGroup->outfits.end())
		isEnd = true;
	innerIter = outerIter->second.begin();
	if (innerIter == outerIter->second.end())
		isEnd = true;

}



// Operators that form allow use with a range-based for loop
bool OutfitGroup::iterator::operator!= (const OutfitGroup::iterator& other) const
{
	if (isEnd && other.isEnd)
		return false;
	return myGroup != other.myGroup || outerIter != other.outerIter || innerIter != other.innerIter;
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
	if (++innerIter == outerIter->second.end())
	{
		if (++outerIter == myGroup->outfits.end())
			isEnd = true;
		else 
			innerIter = outerIter->second.begin();
	}
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



double OutfitGroup::iterator::GetCostRatio() const
{
	return CostFunction(GetAge());
}



std::string OutfitGroup::iterator::GetCostRatioString() const
{
	int64_t maxCost = myGroup->GetCost(GetOutfit(), 1, false);
	int64_t minCost = myGroup->GetCost(GetOutfit(), 1, true);
	int64_t baseCost = GetOutfit()->Cost();
	if (minCost == maxCost)
		return Format::Percent(minCost, baseCost);
	return Format::Percent(minCost, baseCost) + "-" + Format::Percent(maxCost, baseCost);
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
