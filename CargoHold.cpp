/* CargoHold.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CargoHold.h"

#include "GameData.h"
#include "Outfit.h"

#include <algorithm>

using namespace std;



CargoHold::CargoHold()
	: size(0), used(-1)
{
}



void CargoHold::Clear()
{
	size = 0;
	used = -1;
	commodities.clear();
	outfits.clear();
}



// Load the cargo manifest from a DataFile. This must be done after the
// GameData is loaded, so that the sizes of any outfits are known.
void CargoHold::Load(const DataFile::Node &node, const GameData &data)
{
	Clear();
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "commodities")
		{
			for(const DataFile::Node &grand : child)
				if(grand.Size() >= 2)
				{
					int tons = grand.Value(1);
					commodities[grand.Token(0)] += tons;
				}
		}
		else if(child.Token(0) == "outfits")
		{
			for(const DataFile::Node &grand : child)
			{
				const Outfit *outfit = data.Outfits().Get(grand.Token(0));
				int count = (grand.Size() < 2) ? 1 : grand.Value(1);
				outfits[outfit] += count;
			}
		}
	}
}



// Save the cargo manifest to a file.
void CargoHold::Save(std::ostream &out, int depth) const
{
	string prefix(depth, '\t');
	
	bool first = true;
	for(const auto &it : commodities)
		if(it.second)
		{
			if(first)
			{
				out << prefix << "cargo\n";
				out << prefix << "\tcommodities\n";
			}
			first = false;
			
			out << prefix << "\t\t\"" << it.first << "\" " << it.second << "\n";
		}
	
	bool firstOutfit = true;
	for(const auto &it : outfits)
		if(it.second && !it.first->Name().empty())
		{
			// It is possible this cargo hold contained no commodities, meaning
			// we must print the opening tag now.
			if(first)
				out << prefix << "cargo\n";
			first = false;
			
			// If this is the first outfit to be written, print the opening tag.
			if(firstOutfit)
				out << prefix << "\toutfits\n";
			firstOutfit = false;
			
			out << prefix << "\t\t\"" << it.first->Name() << "\" " << it.second << "\n";
		}
}



// Set the capacity of this cargo hold.
void CargoHold::SetSize(int tons)
{
	size = tons;
}



int CargoHold::Size() const
{
	return size;
}



int CargoHold::Free() const
{
	return size - Used();
}



int CargoHold::Used() const
{
	if(used < 0)
	{
		used = 0;
		for(const auto &it : commodities)
			used += it.second;
		for(const auto &it : outfits)
			used += it.second * it.first->Get("mass");
	}
	return used;
}



int CargoHold::CommoditiesSize() const
{
	int size = 0;
	for(const auto &it : commodities)
		size += it.second;
	return size;
}



int CargoHold::OutfitsSize() const
{
	int size = 0;
	for(const auto &it : outfits)
		size += it.second * it.first->Get("mass");
	return size;
}



// Normal cargo:
int CargoHold::Get(const string &commodity) const
{
	map<string, int>::const_iterator it = commodities.find(commodity);
	return (it == commodities.end() ? 0 : it->second);
}



// Spare outfits:
int CargoHold::Get(const Outfit *outfit) const
{
	map<const Outfit *, int>::const_iterator it = outfits.find(outfit);
	return (it == outfits.end() ? 0 : it->second);
}



const map<string, int> &CargoHold::Commodities() const
{
	return commodities;
}



const map<const Outfit *, int> &CargoHold::Outfits() const
{
	return outfits;
}



// For all the transfer functions, the "other" can be null if you simply want
// the commodity to "disappear" or, if the "amount" is negative, to have an
// unlimited supply. The return value is the actual number transferred.
int CargoHold::Transfer(const string &commodity, int amount, CargoHold *to)
{
	// Take your free capacity into account here too.
	amount = min(amount, Get(commodity));
	if(Size())
		amount = max(amount, -Free());
	if(to)
	{
		amount = max(amount, -to->Get(commodity));
		if(to->Size())
			amount = min(amount, to->Free());
	}
	if(!amount)
		return 0;
	
	commodities[commodity] -= amount;
	used -= amount;
	if(to)
	{
		to->commodities[commodity] += amount;
		to->used += amount;
	}
	
	return amount;
}



int CargoHold::Transfer(const Outfit *outfit, int amount, CargoHold *to)
{
	int mass = outfit->Get("mass");
	
	amount = min(amount, Get(outfit));
	if(Size() && mass)
		amount = max(amount, -Free() / mass);
	if(to)
	{
		amount = max(amount, -to->Get(outfit));
		if(to->Size() && mass)
			amount = min(amount, to->Free() / mass);
	}
	if(!amount)
		return 0;
	
	outfits[outfit] -= amount;
	used -= amount * mass;
	if(to)
	{
		to->outfits[outfit] += amount;
		to->used += amount * mass;
	}
	
	return amount;
}



// Transfer as much as the given cargo hold has capacity for. The priority is
// first mission cargo, then spare outfits, then ordinary commodities.
void CargoHold::TransferAll(CargoHold *to)
{
	// If there is no destination specified, just unload everything.
	if(!to)
	{
		commodities.clear();
		outfits.clear();
		used = 0;
		return;
	}
	
	for(const auto &it : outfits)
		Transfer(it.first, it.second, to);
	for(const auto &it : commodities)
		Transfer(it.first, it.second, to);
}


	
// Get the total value of all this cargo, in the given system.
int CargoHold::Value(const System *system) const
{
	int value = 0;
	for(const auto &it : commodities)
		value += system->Trade(it.first) * it.second;
	for(const auto &it : outfits)
		value += it.first->Cost() * it.second;
	return value;
}
