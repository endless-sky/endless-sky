/* Reserves.h
 Copyright (c) 2014 by Michael Zahniser
 
 Endless Sky is free software: you can redistribute it and/or modify it under the
 terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later version.
 
 Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 */

#include "Reserves.h"

#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"

#include <cmath>
#include <algorithm>

using namespace std;



// Reset to the initial political state defined in the game data.
void Reserves::Reset()
{
	amounts.clear();
	recentActivity.clear();
	
	for(const auto &it : GameData::Systems())
		for(const auto &gd : GameData::Commodities())
		{
			amounts[&it.second][gd.name] = it.second.InitialReserves(gd.name);
			recentActivity[&it.second][gd.name] = 0;
		}
}



// Get or set your reputation with the given government.
int64_t Reserves::Amounts(const System *sys, const std::string &commodity) const
{
	auto it = amounts.find(sys);
	return (it == amounts.end() ? 0 : (it->second).find(commodity)->second);
}



// Get or set your reputation with the given government.
int64_t Reserves::RecentActivity(const System *sys, const std::string &commodity) const
{
	auto it = recentActivity.find(sys);
	return (it == recentActivity.end() ? 0 : (it->second).find(commodity)->second);
}



void Reserves::AdjustAmounts(const System *sys, const std::string &commodity, int64_t adjustment, bool recent)
{
	auto it = amounts.find(sys);
	int64_t newVal = max((it->second).find(commodity)->second + adjustment, (int64_t) 0);
	if (it != amounts.end()) (it->second).find(commodity)->second = newVal;
	if (recent)
	{
		it = recentActivity.find(sys);
		if (it != recentActivity.end()) (it->second).find(commodity)->second += adjustment;
	}
}



void Reserves::ReduceRecent(const System *sys, const std::string &commodity)
{
	auto it = recentActivity.find(sys);
	if (it != amounts.end()) (it->second).find(commodity)->second =
		floor(0.95*(it->second).find(commodity)->second);
}



void Reserves::SetAmounts(const System *sys, const std::string &commodity, int64_t adjustment, int64_t recent)
{
	auto it = amounts.find(sys);
	if (it != amounts.end()) (it->second).find(commodity)->second = adjustment;
	it = recentActivity.find(sys);
	if (it != recentActivity.end()) (it->second).find(commodity)->second = recent;
}



// Reset any temporary provocation (typically because a day has passed).
void Reserves::EvolveDaily()
{
	for(const auto &it : GameData::Systems())
		for(const auto &gd : GameData::Commodities())
		{
			AdjustAmounts(&it.second, gd.name,
				it.second.Production(gd.name) + it.second.Trading(gd.name) - it.second.Consumption(gd.name), false);
			ReduceRecent(&it.second, gd.name);
		}
}