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



// Operators that form allow use with a range-based for loop
bool OutfitGroup::iterator::operator!= (const OutfitGroup::iterator& other) const
{
	return position != other.position;
}

int OutfitGroup::iterator::operator* () const
{
	return myGroup->get();
}

const OutfitGroup::iterator& OutfitGroup::iterator::operator++ ()
{
	if (innerIter == )
	return *this;
}





void OutfitGroup::Save(DataWriter &out) const
{
	
}



int64_t OutfitGroup::GetTotalCost() const
{
	
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

