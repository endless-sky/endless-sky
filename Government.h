/* Government.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GOVERNMENT_H_
#define GOVERNMENT_H_

#include "Color.h"

#include <map>
#include <string>

class DataNode;



// Class representing a government. All ships have a government, and each
// government can have others as allies (those it will assist if they are under
// attack) or enemies (those it will attack without provocation). These relations
// need not be mutual. For example, pirates attack merchants, but merchants do not
// seek out fights with pirates; Republic warships defend merchants who are under
// attack, but merchants do not join in when Republic ships are fighting against
// Free Worlds ships.
class Government {
public:
	// Default constructor.
	Government();
	
	// Load a government's definition from a file.
	void Load(const DataNode &node);
	
	// Get the name of this government.
	const std::string &GetName() const;
	// Get the color swizzle to use for ships of this government.
	int GetSwizzle() const;
	// Get the color to use for displaying this government on the map.
	const Color &GetColor() const;
	
	// Get the government's initial disposition toward other governments or
	// toward the player.
	double AttitudeToward(const Government *other) const;
	double InitialPlayerReputation() const;
	// Get the amount that your reputation changes for the given offense. The
	// given value should be a combination of one or more ShipEvent values.
	double PenaltyFor(int eventType) const;
	// In order to successfully bribe this government you must pay them this
	// fraction of your fleet's value. (Zero means they cannot be bribed.)
	double GetBribeFraction() const;
	
	// Check if, according to the politics stored by GameData, this government is
	// an enemy of the given government right now.
	bool IsEnemy(const Government *other) const;
	
	
private:
	std::string name;
	int swizzle;
	Color color;
	
	std::map<const Government *, double> attitudeToward;
	double initialPlayerReputation;
	std::map<int, double> penaltyFor;
	double bribe;
};



#endif
