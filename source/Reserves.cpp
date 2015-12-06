/* Reserves.h
 Copyright (c) 2015 by Michael Zahniser and James Guillochon
 
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



// Reset to the initial commodity reserve state, which is calculated from
// the price of the commodity set in each system.
void Reserves::Reset()
{
	amounts.clear();
	
	for(const auto &it : GameData::Systems())
		for(const auto &gd : GameData::Commodities())
			amounts[&it.second][gd.name] = it.second.InitialReserves(gd.name);
}



// Get the amount of commodity reserved in a given system.
int64_t Reserves::Amounts(const System *sys, const std::string &commodity) const
{
	auto it = amounts.find(sys);
	return (it == amounts.end() ? 0 : (it->second).find(commodity)->second);
}



// Adjust the amount of commodity in a given system.
void Reserves::AdjustAmounts(const System *sys, const std::string &commodity, int64_t adjustment)
{
	auto it = amounts.find(sys);
	int64_t newVal = max((it->second).find(commodity)->second + adjustment, (int64_t) 0);
	if (it != amounts.end()) (it->second).find(commodity)->second = newVal;
}



// Set the amount of commodity in a given system, and the amount that was recently
// transcated by the player.
void Reserves::SetAmounts(const System *sys, const std::string &commodity, int64_t adjustment)
{
	auto it = amounts.find(sys);
	if (it != amounts.end()) (it->second).find(commodity)->second = adjustment;
}



// Evolve the amount of commodities available in each system through a combination
// of production, consumption, and trade. This function is run once per day.
void Reserves::EvolveDaily()
{
	for(const auto &it : GameData::Systems())
		for(const auto &gd : GameData::Commodities())
		{
			AdjustAmounts(&it.second, gd.name,
				it.second.Production(gd.name) + it.second.Trading(gd.name) -
				it.second.Consumption(gd.name) - it.second.BlessingsAndDisasters(gd.name));
		}
}