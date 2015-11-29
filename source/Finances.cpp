/* Finances.h
 Copyright (c) 2014 by Michael Zahniser
 
 Endless Sky is free software: you can redistribute it and/or modify it under the
 terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later version.
 
 Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 */

#include "Finances.h"

#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"

#include <algorithm>

using namespace std;



// Reset to the initial political state defined in the game data.
void Finances::Reset()
{

}



// Get or set your reputation with the given government.
double Finances::Reserves(const System *sys) const
{
	auto it = reserves.find(sys);
	return (it == reserves.end() ? 0. : it->second);
}



void Finances::AdjustReserves(const System *sys, const std::string &commodity, int64_t adjustment)
{
	reserves[commodity] += adjustment;
}



void Finances::SetReserves(const System *sys, const std::string &commodity, int64_t adjustment)
{
	reserves[sys][commodity] = adjustment;
}
