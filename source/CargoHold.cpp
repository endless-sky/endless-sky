/* CargoHold.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "CargoHold.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Depreciation.h"
#include "GameData.h"
#include "Government.h"
#include "Mission.h"
#include "Outfit.h"
#include "System.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	// Retrieve vector of pointers to the outfits, sorted descending by size.
	vector<const Outfit *> OrderOutfitsBySize(const map<const Outfit *, int> &outfits)
	{
		vector<const Outfit *> sortedOutfits;
		for(const auto &it : outfits)
			sortedOutfits.emplace_back(it.first);

		sort(sortedOutfits.begin(), sortedOutfits.end(),
			[](const Outfit *lhs, const Outfit *rhs)
			{
				return lhs->Mass() > rhs->Mass();
			}
		);

		return sortedOutfits;
	}
}



// Remove any items in this cargo hold.
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
	// Cargo is stored as name / amount pairs in two lists: commodities and outfits.
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
			// Only write a "cargo" block if it is not going to be empty.
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
	// We only need to EndChild() if at least one line was written above.
	if(!first)
		out.EndChild();

	// Save all outfits, even ones which have only been referred to.
	bool firstOutfit = true;
	for(const auto &it : outfits)
		if(it.second)
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

			out.Write(it.first->TrueName(), it.second);
		}
	// Back out any indentation blocks that are set, depending on what sorts of
	// cargo were written to the file.
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



// Get the capacity of this hold.
int CargoHold::Size() const
{
	return size;
}



// Get the amount of free cargo space.
int CargoHold::Free() const
{
	return size - Used();
}



double CargoHold::FreePrecise() const
{
	return size - UsedPrecise();
}



// Get the total amount of cargo space used, rounded up to the nearest ton.
// (Some outfits may have non-integral masses.)
int CargoHold::Used() const
{
	return CommoditiesSize() + OutfitsSize() + MissionCargoSize();
}



double CargoHold::UsedPrecise() const
{
	return CommoditiesSize() + OutfitsSizePrecise() + MissionCargoSize();
}



// Get the total number of tons of commodities.
int CargoHold::CommoditiesSize() const
{
	int size = 0;
	for(const auto &it : commodities)
		size += it.second;
	return size;
}



// Get the total mass of outfit cargo, rounded up to the nearest ton.
int CargoHold::OutfitsSize() const
{
	return ceil(OutfitsSizePrecise());
}



double CargoHold::OutfitsSizePrecise() const
{
	double size = 0.;
	for(const auto &it : outfits)
		size += it.second * it.first->Mass();
	return size;
}



// Check if any outfits are being carried. Note that some outfits may have mass
// zero, so this check cannot be done by calling OutfitsSize().
bool CargoHold::HasOutfits() const
{
	// The code for adding and removing outfits does not clear the entry in the
	// map if its value becomes zero, so we need to check all the entries:
	for(const auto &it : outfits)
		if(it.second)
			return true;

	return false;
}



// Get the total mass of mission cargo.
int CargoHold::MissionCargoSize() const
{
	int size = 0;
	for(const auto &it : missionCargo)
		size += it.second;
	return size;
}



// Check if any mission cargo is being carried. Some mission cargo has no mass,
// so this cannot be done by calling MissionCargoSize().
bool CargoHold::HasMissionCargo() const
{
	return !missionCargo.empty();
}



// Check if there is anything in this cargo hold (including passengers).
bool CargoHold::IsEmpty() const
{
	// The outfits map's entries are not erased if they are equal to zero, so
	// it's not enough to just test outfits.empty(). Same goes for commodities.
	return !CommoditiesSize() && !HasOutfits() && missionCargo.empty() && passengers.empty();
}



// Set the number of free bunks for passengers.
void CargoHold::SetBunks(int count)
{
	bunks = count;
}



// Check how many bunks are free.
int CargoHold::BunksFree() const
{
	return bunks - Passengers();
}



// Check how many bunks are occupied by passengers.
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



// Spare outfits (including plunder and mined materials):
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



// Check how many passengers for the given mission are being carried.
int CargoHold::GetPassengers(const Mission *mission) const
{
	map<const Mission *, int>::const_iterator it = passengers.find(mission);
	return (it == passengers.end() ? 0 : it->second);
}



// Access the commodities map directly.
const map<string, int> &CargoHold::Commodities() const
{
	return commodities;
}



// Access the outfits map directly.
const map<const Outfit *, int> &CargoHold::Outfits() const
{
	return outfits;
}



// Access the mission cargo map directly.
const map<const Mission *, int> &CargoHold::MissionCargo() const
{
	return missionCargo;
}



// Access the mission passenger map directly.
const map<const Mission *, int> &CargoHold::PassengerList() const
{
	return passengers;
}



// Transfer ordinary commodities from one cargo hold to another.
int CargoHold::Transfer(const string &commodity, int amount, CargoHold &to)
{
	if(!amount)
		return 0;

	// Remove up to the specified tons of cargo from this cargo hold, adding
	// them to the given cargo hold if possible. If not possible, add the
	// remainder back to this cargo hold, even if there is no space for it.
	// Do not invalidate existing iterators by modifying the container.
	int removed = Remove(commodity, amount);
	int added = to.Add(commodity, removed);
	commodities[commodity] += removed - added;

	return added;
}



// Transfer outfits from one cargo hold to another.
int CargoHold::Transfer(const Outfit *outfit, int amount, CargoHold &to)
{
	if(!amount)
		return 0;

	// Remove up to the specified number of items from this cargo hold, adding
	// them to the given cargo hold if possible. If not possible, add the
	// remainder back to this cargo hold, even if there is no space for it.
	// Do not invalidate existing iterators by modifying the container.
	int removed = Remove(outfit, amount);
	int added = to.Add(outfit, removed);
	outfits[outfit] += removed - added;

	return added;
}



// Transfer mission cargo from one cargo hold to another.
int CargoHold::Transfer(const Mission *mission, int amount, CargoHold &to)
{
	// A negative amount means a transfer in the opposite direction.
	if(amount < 0)
		return -to.Transfer(mission, -amount, *this);

	// If transferring 0 cargo, don't create an entry in the destination cargo
	// hold unless told to transfer 0 and the amount in this cargo hold is 0.
	int existing = Get(mission);
	if(amount && !existing)
		return 0;
	amount = min(amount, existing);
	if(to.size >= 0)
		amount = max(0, min(amount, to.Free()));
	// Don't transfer 0 tons unless that's all that exists.
	if(existing && !amount)
		return 0;

	missionCargo[mission] -= amount;
	to.missionCargo[mission] += amount;

	return amount;
}



// Transfer mission passengers from one cargo hold to another.
int CargoHold::TransferPassengers(const Mission *mission, int amount, CargoHold &to)
{
	// A negative amount means a transfer in the opposite direction.
	if(amount < 0)
		return -to.TransferPassengers(mission, -amount, *this);

	// Check if the destination cargo hold has a limit on the number of bunks.
	amount = min(amount, GetPassengers(mission));
	if(to.bunks >= 0)
		amount = max(0, min(amount, to.BunksFree()));

	if(amount)
	{
		passengers[mission] -= amount;
		to.passengers[mission] += amount;
	}
	return amount;
}



// Transfer as much as the given cargo hold has capacity for. The priority is
// first mission cargo, then spare outfits, then ordinary commodities.
void CargoHold::TransferAll(CargoHold &to, bool transferPassengers)
{
	if(transferPassengers)
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
	const vector<const Outfit *> outfitOrder = OrderOutfitsBySize(outfits);
	for(const auto &outfit : outfitOrder)
		Transfer(outfit, outfits[outfit], to);
	for(const auto &it : commodities)
		Transfer(it.first, it.second, to);
}



// Add the given amount of the given commodity.
int CargoHold::Add(const string &commodity, int amount)
{
	if(amount < 0)
		return -Remove(commodity, -amount);

	// If this cargo hold has a size limit, apply it.
	if(size >= 0)
		amount = max(0, min(amount, Free()));
	commodities[commodity] += amount;
	return amount;
}



// Add the given number of copies of the given outfit.
int CargoHold::Add(const Outfit *outfit, int amount)
{
	if(amount < 0)
		return -Remove(outfit, -amount);

	// If the outfit has mass and this cargo hold has a size limit, apply it.
	double mass = outfit->Mass();
	if(size >= 0 && mass > 0.)
		amount = max(0, min(amount, static_cast<int>(FreePrecise() / mass)));
	outfits[outfit] += amount;
	return amount;
}



// Remove the given amount of the given commodity.
int CargoHold::Remove(const string &commodity, int amount)
{
	if(amount < 0)
		return Add(commodity, -amount);

	amount = min(amount, commodities[commodity]);
	commodities[commodity] -= amount;
	return amount;
}



// Remove the given number of copies of the given outfit.
int CargoHold::Remove(const Outfit *outfit, int amount)
{
	if(amount < 0)
		return Add(outfit, -amount);

	amount = min(amount, outfits[outfit]);
	outfits[outfit] -= amount;
	return amount;
}



// Add all the cargo and passengers associated with the given mission.
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



// Remove all the cargo and passengers (if any) associated with the given mission.
void CargoHold::RemoveMissionCargo(const Mission *mission)
{
	missionCargo.erase(mission);
	passengers.erase(mission);
}



// Get the total value of all this cargo, in the given system.
int64_t CargoHold::Value(const System *system) const
{
	int64_t value = 0;
	for(const auto &it : commodities)
		value += static_cast<int64_t>(system->Trade(it.first)) * it.second;
	// For outfits, assume they're fully depreciated, since that will always be
	// the case unless the player bought into cargo for some reason.
	for(const auto &it : outfits)
		value += it.first->Cost() * it.second * Depreciation::Full();
	return value;
}



// If anything you are carrying is illegal, return the maximum fine you can
// be charged for any illegal outfits plus the sum of the fines for all
// missions. If the returned value is negative, you are carrying something so
// bad that it warrants a death sentence.
int CargoHold::IllegalCargoFine(const Government *government, const PlayerInfo &player) const
{
	int totalFine = 0;
	// Carrying an illegal outfit is only half as bad as having it equipped.
	// Only the worst illegal outfit is fined.
	for(const auto &it : outfits)
	{
		// The code for adding and removing outfits does not clear the entry in the
		// map if its value becomes zero, so we need to check if the outfit is
		// actually inside the cargo hold.
		if(!it.second)
			continue;

		int fine = government->Fines(it.first);
		if(government->Condemns(it.first))
			return -1;
		if(fine < 0)
			return fine;
		totalFine = max(totalFine, fine / 2);
	}

	// Fines for illegal mission cargo and passengers are added together to
	// avoid the player being able to stack multiple illegal jobs at once
	// and avoid the bulk of the penalties when fined.
	for(const auto &it : missionCargo)
	{
		int fine = it.first->Fine();
		if(fine < 0)
			return fine;
		if(!it.first->IsFailed(player))
			totalFine += fine;
	}

	return totalFine;
}



int CargoHold::IllegalPassengersFine(const Government *government, const PlayerInfo &player) const
{
	int totalFine = 0;
	for(const auto &it : passengers)
	{
		int fine = it.first->Fine();
		if(fine < 0)
			return fine;
		if(!it.first->IsFailed(player))
			totalFine += fine;
	}

	return totalFine;
}



// Returns the amount tons of illegal cargo.
int CargoHold::IllegalCargoAmount() const
{
	int count = 0;

	// Find any illegal outfits inside the cargo hold.
	for(const auto &it : outfits)
		if(it.first->Get("illegal") || it.first->Get("atrocity") > 0.)
			count += it.second * max(0., it.first->Mass() + it.first->Get("scan brightness"));

	// Find any illegal mission cargo.
	for(const auto &it : missionCargo)
		if(it.first->Fine())
			count += it.second;

	return count;
}
