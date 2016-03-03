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
	
}



int64_t OutfitGroup::GetTotalCost(const Outfit *outfit) const
{
	
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
			cost += CostFunction(outfit->Cost(), it->first);	
			if (--count <= 0)
				break;
		}
	}
	else 
	{ // newest first
		auto it = matchingOutfits->second.begin();
		for (; it != matchingOutfits->second.end(); ++it)
		{
			cost += CostFunction(outfit->Cost(), it->first);	
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
