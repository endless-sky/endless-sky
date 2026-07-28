/* Depreciation.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Depreciation.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Gamerules.h"
#include "Outfit.h"
#include "Ship.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Methods to retrieve depreciation constants from gamerules.
	double Min()
	{
		return GameData::GetGamerules().DepreciationMin();
	}

	int GracePeriod()
	{
		return GameData::GetGamerules().DepreciationGracePeriod();
	}

	double Daily()
	{
		return GameData::GetGamerules().DepreciationDaily();
	}

	int MaxAge()
	{
		return GameData::GetGamerules().DepreciationMaxAge() + GracePeriod();
	}

	// Names for the two kinds of depreciation records.
	string NAME[2] = {"fleet depreciation", "stock depreciation"};
}



// What fraction of its cost a fully depreciated item has left:
double Depreciation::Full()
{
	return Min();
}



// Load depreciation records.
void Depreciation::Load(const DataNode &node)
{
	// Check if this is fleet or stock depreciation.
	isStock = (node.Token(0) == NAME[1]);
	isLoaded = true;

	for(const DataNode &child : node)
	{
		bool isShip = (child.Token(0) == "ship");
		bool isOutfit = (child.Token(0) == "outfit");
		if(!(isShip || isOutfit) || child.Size() < 2)
			continue;

		// Figure out which record we're modifying.
		map<int, int> &entry = isShip ?
			ships[GameData::Ships().Get(child.Token(1))] :
			outfits[GameData::Outfits().Get(child.Token(1))];

		// Load any depreciation records for this item.
		for(const DataNode &grand : child)
			if(grand.Size() >= 2)
				entry[grand.Value(0)] += grand.Value(1);
	}
}



// Save depreciation records.
void Depreciation::Save(DataWriter &out, int day) const
{
	out.Write(NAME[isStock]);
	out.BeginChild();
	{
		using ShipElement = pair<const Ship *const, map<int, int>>;
		WriteSorted(ships,
			[](const ShipElement *lhs, const ShipElement *rhs)
				{ return lhs->first->TrueModelName() < rhs->first->TrueModelName(); },
			[=, this, &out](const ShipElement &sit)
			{
				out.Write("ship", sit.first->TrueModelName());
				out.BeginChild();
				{
					// If this is a planet's stock, remember how many outfits in
					// stock are fully depreciated. If it's the player's stock,
					// anything not recorded is considered fully depreciated, so
					// there is no reason to save records for those items.
					for(const auto &it : sit.second)
						if(isStock || (it.second && it.first > day - MaxAge()))
							out.Write(it.first, it.second);
				}
				out.EndChild();
			});
		using OutfitElement = pair<const Outfit *const, map<int, int>>;
		WriteSorted(outfits,
			[](const OutfitElement *lhs, const OutfitElement *rhs)
				{ return lhs->first->TrueName() < rhs->first->TrueName(); },
			[=, this, &out](const OutfitElement &oit)
			{
				out.Write("outfit", oit.first->TrueName());
				out.BeginChild();
				{
					for(const auto &it : oit.second)
						if(isStock || (it.second && it.first > day - MaxAge()))
							out.Write(it.first, it.second);
				}
				out.EndChild();
			});
	}
	out.EndChild();
}



// Check if any records have been loaded.
bool Depreciation::IsLoaded() const
{
	return isLoaded;
}



// If no records have been loaded, initialize with an entire fleet.
void Depreciation::Init(const vector<shared_ptr<Ship>> &fleet, int day)
{
	// If this is called, this is a player's fleet, not a planet's stock.
	isStock = false;
	// Every ship and outfit in the given fleet starts out with no depreciation.
	for(const shared_ptr<Ship> &ship : fleet)
	{
		const Ship *base = GameData::Ships().Get(ship->TrueModelName());
		++ships[base][day];

		for(const auto &it : ship->Outfits())
			outfits[it.first][day] += it.second;
	}
}



// Add a ship, and all its outfits, to the depreciation record.
void Depreciation::Buy(const Ship &ship, int day, Depreciation *source, bool chassisOnly)
{
	// First, add records for all outfits the ship is carrying.
	if(!chassisOnly)
		for(const auto &it : ship.Outfits())
			for(int i = 0; i < it.second; ++i)
				Buy(it.first, day, source);

	// Then, check the base day for the ship chassis itself.
	const Ship *base = GameData::Ships().Get(ship.TrueModelName());
	if(source)
	{
		// Check if the source has any instances of this ship.
		auto it = source->ships.find(base);
		if(it != source->ships.end() && !it->second.empty())
		{
			day = source->Sell(it->second);
			if(it->second.empty())
				source->ships.erase(it);
		}
		else if(isStock)
		{
			// If we're a planet buying from the player, and the player has no
			// record of how old this ship is, it's fully depreciated.
			day -= MaxAge();
		}
	}

	// Increment our count for this ship on this day.
	++ships[base][day];
}



// Add a single outfit to the depreciation record.
void Depreciation::Buy(const Outfit *outfit, int day, Depreciation *source)
{
	if(outfit->Get("installable") < 0.)
		return;

	if(source)
	{
		// Check if the source has any instances of this outfit.
		auto it = source->outfits.find(outfit);
		if(it != source->outfits.end() && !it->second.empty())
		{
			day = source->Sell(it->second);
			if(it->second.empty())
				source->outfits.erase(it);
		}
		else if(isStock)
		{
			// If we're a planet buying from the player, and the player has no
			// record of how old this outfit is, it's fully depreciated.
			day -= MaxAge();
		}
	}

	// Increment our count for this outfit on this day.
	++outfits[outfit][day];
}



// Get the value of an entire fleet.
int64_t Depreciation::Value(const vector<shared_ptr<Ship>> &fleet, int day, bool chassisOnly) const
{
	map<const Ship *, int> shipCount;
	map<const Outfit *, int> outfitCount;

	for(const shared_ptr<Ship> &ship : fleet)
	{
		const Ship *base = GameData::Ships().Get(ship->TrueModelName());
		++shipCount[base];

		if(!chassisOnly)
			for(const auto &it : ship->Outfits())
				outfitCount[it.first] += it.second;
	}

	int64_t value = 0;
	for(const auto &it : shipCount)
		value += Value(it.first, day, it.second);
	for(const auto &it : outfitCount)
		value += Value(it.first, day, it.second);
	return value;
}



// Get the value of a ship, along with all its outfits.
int64_t Depreciation::Value(const Ship &ship, int day) const
{
	int64_t value = Value(&ship, day);
	for(const auto &it : ship.Outfits())
		value += Value(it.first, day, it.second);
	return value;
}



// Get the value just of the chassis of a ship.
int64_t Depreciation::Value(const Ship *ship, int day, int count) const
{
	// Check whether a record exists for this ship. If not, its value is full
	// if this is  planet's stock, or fully depreciated if this is the player.
	ship = GameData::Ships().Get(ship->TrueModelName());
	auto recordIt = ships.find(ship);
	if(recordIt == ships.end() || recordIt->second.empty())
		return DefaultDepreciation() * count * ship->ChassisCost();

	return Depreciate(recordIt->second, day, count) * ship->ChassisCost();
}



// Get the value of an outfit.
int64_t Depreciation::Value(const Outfit *outfit, int day, int count) const
{
	if(outfit->Get("installable") < 0.)
		return count * outfit->Cost();

	// Check whether a record exists for this outfit. If not, its value is full
	// if this is  planet's stock, or fully depreciated if this is the player.
	auto recordIt = outfits.find(outfit);
	if(recordIt == outfits.end() || recordIt->second.empty())
		return DefaultDepreciation() * count * outfit->Cost();

	return Depreciate(recordIt->second, day, count) * outfit->Cost();
}



// "Sell" an item, removing it from the given record and returning the base
// day for its depreciation.
int Depreciation::Sell(map<int, int> &record) const
{
	// If we're a planet, we start by selling the oldest, cheapest thing.
	auto it = (isStock ? record.begin() : --record.end());
	int day = it->first;

	// Remove one record from the source. If necessary, delete this
	// record line or the entire record for this outfit.
	--it->second;
	if(!it->second)
		record.erase(it);

	return day;
}



// Calculate depreciation for some number of items.
double Depreciation::Depreciate(const map<int, int> &record, int day, int count) const
{
	if(record.empty())
		return count * DefaultDepreciation();

	// Depending on whether this is a planet's stock or a player's fleet, we
	// should either start with the oldest item, or the newest.
	map<int, int>::const_iterator it = (isStock ? record.begin() : --record.end());

	double sum = 0.;
	while(true)
	{
		// Check whether there are enough items in this particular bin to use up
		// the entire remaining count, and add the depreciation amount for
		// however many items from this bin we can use.
		int used = min(it->second, count);
		count -= used;
		sum += used * Depreciate(day - it->first);
		// Bail out if we've counted enough items.
		if(!count)
			break;

		// Increment the iterator in the proper direction.
		if(isStock)
		{
			++it;
			if(it == record.end())
				break;
		}
		else
		{
			if(it == record.begin())
				break;
			--it;
		}
	}
	// For all items we don't have a record for, apply the default depreciation.
	return sum + count * DefaultDepreciation();
}



// Calculate the value fraction for an item of the given age.
double Depreciation::Depreciate(int age) const
{
	if(age <= GracePeriod())
		return 1.;

	if(age >= MaxAge())
		return Min();

	int effectiveAge = age - GracePeriod();
	double daily = pow(Daily(), effectiveAge);
	double linear = static_cast<double>(MaxAge() - effectiveAge) / MaxAge();
	return Min() + (1. - Min()) * daily * linear;
}



// Depreciation of an item for which no record exists. If buying, items
// default to no depreciation. When selling, they default to full.
double Depreciation::DefaultDepreciation() const
{
	return (isStock ? 1. : Min());
}
