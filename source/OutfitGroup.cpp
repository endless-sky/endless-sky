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



bool OutfitGroup::Enpty() const
{
	return outfits.empty();
}



std::map<int64_t, int> OutfitGroup::Find(const Outfit *outfit) const
{
	return outfits.find(outfit);
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
	map<int64_t, int> matchingOutfits = outfits.Find(outfit);
	if (matchingOutfits == outfits.End())
		return 0;
	for (auto it : matchingOutfits.second)
		count += it.second;
	return count;
}



int64_t OutfitGroup::GetCost(const Outfit* outfit, int count, bool oldestFirst) const
{
	
}



void OutfitGroup::AddOutfit(const Outfit* outfit, int count, int age)
{
	
}



void OutfitGroup::RemoveOutfit(const Outfit* outfit, int count, bool oldestFirst)
{
	
}



void OutfitGroup::TransferOutfits(const Outfit *outfit, int count, OutfitGroup* to)
{
	
}



void OutfitGroup::IncrementDate()
{
	
}



// Operators that form allow use with a range-based for loop
bool OutfitGroup::iterator::operator!= (const OutfitGroup::iterator& other) const
{
	return position != other.position;
}



 OutfitGroup::iterator::operator* () const
{
	return 0;
}



const OutfitGroup::iterator& OutfitGroup::iterator::operator++ ()
{
	if (innerIter == currentOutfit.end())
	{
		outerIter++;
		innerIter = outerIter.second().begin();
	}
	else
		innerIter++;
	
	return *this;
}



const Outfit* OutfitGroup::iterator::GetOutfit() const
{
	return outerIter.first();
}



int64_t OutfitGroup::iterator::GetAge() const
{
	return innerIter.first();
}



int OutfitGroup::iterator::GetQuantity() const
{
	return innerIter.second();
}
