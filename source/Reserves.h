/* Reserves.h
 Copyright (c) 2015 by Michael Zahniser and James Guillochon
 
 Endless Sky is free software: you can redistribute it and/or modify it under the
 terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later version.
 
 Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 */

#ifndef RESERVES_H_
#define RESERVES_H_

#include <map>
#include <set>
#include <string>

class System;



// This class represents the current state of commodity holdings within each system.
// Each system has a finite amount of each commodity, which is determined by the
// system's commodity price at the start of the game. As commodities are traded
// between worlds by the player, and as time elapses and goods are produced, consumed,
// and traded between systems, the prices of the commodities slowly adjust to
// a new equilibrium. In the future this system could be modified to account for other
// factors, such as events that could suddenly reduce the amount of goods in a system.
class Reserves {
public:
	// Reset to the initial political state defined in the game data.
	void Reset();
	
	// Get or set the amount of commodity in a given system.
	int64_t Amounts(const System *sys, const std::string &commodity) const;
	void AdjustAmounts(const System *sys, const std::string &commodity, int64_t adjustment);
	void SetAmounts(const System *sys, const std::string &commodity, int64_t adjustment);
	
	// Functions that affect the amount of commodity in a system as time elapses.
	void EvolveDaily();
	
private:
	// amounts stores the amount of each commodity in each system.
	std::map<const System *, std::map<std::string, int64_t>> amounts;
};



#endif
