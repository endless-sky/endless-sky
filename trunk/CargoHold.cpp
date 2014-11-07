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

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Mission.h"
#include "Outfit.h"

#include <algorithm>

using namespace std;



void CargoHold::Clear()
{
	size = 0;
	bunks = 0;
	commodities.clear();
	outfits.clear();
	missionCargo.clear();
	passengers.clear();
}



// Load the cargo manifest from a DataFile. This must be done after the
// GameData is loaded, so that the sizes of any outfits are known.
void CargoHold::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "commodities")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
				{
					int tons = grand.Value(1);
					commodities[grand.Token(0)] += tons;
				}
		}
		else if(child.Token(0) == "outfits")
		{
			for(const DataNode &grand : child)
			{
				const Outfit *outfit = GameData::Outfits().Get(grand.Token(0));
				int count = (grand.Size() < 2) ? 1 : grand.Value(1);
				outfits[outfit] += count;
			}
		}
	}
}



// Save the cargo manifest to a file.
void CargoHold::Save(DataWriter &out) const
{
	bool first = true;
	for(const auto &it : commodities)
		if(it.second)
		{
			if(first)
			{
				out.Write("cargo");
				out.BeginChild();
				out.Write("commodities");
				out.BeginChild();
			}
			first = false;
			
			out.Write(it.first, it.second);
		}
	if(!first)
		out.EndChild();
	
	bool firstOutfit = true;
	for(const auto &it : outfits)
		if(it.second && !it.first->Name().empty())
		{
			// It is possible this cargo hold contained no commodities, meaning
			// we must print the opening tag now.
			if(first)
			{
				out.Write("cargo");
				out.BeginChild();
			}
			first = false;
			
			// If this is the first outfit to be written, print the opening tag.
			if(firstOutfit)
			{
				out.Write("outfits");
				out.BeginChild();
			}
			firstOutfit = false;
			
			out.Write(it.first->Name(), it.second);
		}
	if(!firstOutfit)
		out.EndChild();
	if(!first)
		out.EndChild();
	
	// Mission cargo is not saved because it is repopulated when the missions
	// are read rather than when the cargo is read.
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
	return max(0, size - Used());
}



int CargoHold::Used() const
{
	int used = 0;
	for(const auto &it : commodities)
		used += it.second;
	for(const auto &it : outfits)
		used += it.second * it.first->Get("mass");
	for(const auto &it : missionCargo)
		used += it.second;
	
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



bool CargoHold::HasOutfits() const
{
	for(const auto &it : outfits)
		if(it.second)
			return true;
	return false;
}



int CargoHold::MissionCargoSize() const
{
	int size = 0;
	for(const auto &it : missionCargo)
		size += it.second;
	return size;
}



bool CargoHold::HasMissionCargo() const
{
	return !missionCargo.empty();
}



bool CargoHold::IsEmpty() const
{
	return commodities.empty() && outfits.empty() && missionCargo.empty() && passengers.empty();
}



// Set the number of free bunks for passengers.
void CargoHold::SetBunks(int count)
{
	bunks = count;
}



int CargoHold::Bunks() const
{
	return bunks - Passengers();
}



int CargoHold::Passengers() const
{
	int count = 0;
	for(const auto &it : passengers)
		count += it.second;
	return count;
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



// Mission cargo:
int CargoHold::Get(const Mission *mission) const
{
	map<const Mission *, int>::const_iterator it = missionCargo.find(mission);
	return (it == missionCargo.end() ? 0 : it->second);
}



int CargoHold::GetPassengers(const Mission *mission) const
{
	map<const Mission *, int>::const_iterator it = passengers.find(mission);
	return (it == passengers.end() ? 0 : it->second);
}



const map<string, int> &CargoHold::Commodities() const
{
	return commodities;
}



const map<const Outfit *, int> &CargoHold::Outfits() const
{
	return outfits;
}



const map<const Mission *, int> &CargoHold::MissionCargo() const
{
	return missionCargo;
}



const map<const Mission *, int> &CargoHold::PassengerList() const
{
	return passengers;
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
	if(to)
		to->commodities[commodity] += amount;
	
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
	if(to)
		to->outfits[outfit] += amount;
	
	return amount;
}



int CargoHold::Transfer(const Mission *mission, int amount, CargoHold *to)
{
	// Special case: if the mission cargo has zero size, always transfer it. But
	// if it has nonzero size and zero can fit, do _not_ transfer it.
	if(amount)
	{
		// Take your free capacity into account here too.
		amount = min(amount, Get(mission));
		if(Size())
			amount = max(amount, -Free());
		if(to)
		{
			amount = max(amount, -to->Get(mission));
			if(to->Size())
				amount = min(amount, to->Free());
		}
		if(!amount)
			return 0;
	}
	
	missionCargo[mission] -= amount;
	if(to)
		to->missionCargo[mission] += amount;
	
	return amount;
}



int CargoHold::TransferPassengers(const Mission *mission, int amount, CargoHold *to)
{
	// Take your free capacity into account here too.
	amount = min(amount, GetPassengers(mission));
	if(Size())
		amount = max(amount, -Bunks());
	if(to)
	{
		amount = max(amount, -to->GetPassengers(mission));
		if(to->Size())
			amount = min(amount, to->Bunks());
	}
	if(!amount)
		return 0;
	
	passengers[mission] -= amount;
	if(to)
		to->passengers[mission] += amount;
	
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
		missionCargo.clear();
		return;
	}
	
	for(const auto &it : passengers)
		TransferPassengers(it.first, it.second, to);
	// Handle zero-size mission cargo correctly. For mission cargo, having an
	// entry in the map, but with a size of zero, is different than not having
	// an entry at all.
	auto mit = missionCargo.begin();
	while(mit != missionCargo.end())
	{
		Transfer(mit->first, mit->second, to);
		if(!mit->second)
			mit = missionCargo.erase(mit);
		else
			++mit;
	}
	for(const auto &it : outfits)
		Transfer(it.first, it.second, to);
	for(const auto &it : commodities)
		Transfer(it.first, it.second, to);
}



void CargoHold::AddMissionCargo(const Mission *mission)
{
	// If the mission defines a cargo string, create an entry for it even if the
	// cargo size is zero. This is so that, for example, your cargo listing can
	// show "important documents" even if the documents take up no cargo space.
	if(mission && !mission->Cargo().empty())
		missionCargo[mission] += mission->CargoSize();
	if(mission && mission->Passengers())
		passengers[mission] += mission->Passengers();
}



void CargoHold::RemoveMissionCargo(const Mission *mission)
{
	auto it = missionCargo.find(mission);
	if(it != missionCargo.end())
		missionCargo.erase(it);
	
	auto pit = passengers.find(mission);
	if(pit != passengers.end())
		passengers.erase(pit);
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


	
// If anything you are carrying is illegal, return the maximum fine you can
// be charged. If the returned value is negative, you are carrying something
// so bad that it warrants a death sentence.
int CargoHold::IllegalCargoFine() const
{
	int worst = 0;
	// Carrying an illegal outfit is only half as bad as having it equipped.
	for(const auto &it : outfits)
	{
		int fine = it.first->Get("illegal");
		if(fine < 0)
			return fine;
		worst = max(worst, fine / 2);
	}
	
	for(const auto &it : missionCargo)
	{
		int fine = it.first->IllegalCargoFine();
		if(fine < 0)
			return fine;
		worst = max(worst, fine);
	}
	return worst;
}
