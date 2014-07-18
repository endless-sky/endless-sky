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
#include "Set.h"

#include <set>
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
	
	// Check whether ships of this government will come to the aid of ships of
	// the given government that are under attack.
	bool IsAlly(const Government *other) const;
	// Check whether ships of this government will preemptively attack ships of
	// the given government.
	bool IsEnemy(const Government *other) const;
	
	// Mark that this government is, for the moment, fighting the given
	// government, which is not necessarily one of its normal enemies, because
	// a ship of that government attacked it or one of its allies.
	void Provoke(const Government *other, double damage) const;
	// Check if we are provoked against the given government.
	bool IsProvoked(const Government *other) const;
	// Reset the record of who has provoked whom.
	void ResetProvocation() const;
	// Every time step, the provokation values fade a little:
	void CoolDown() const;
	
	
private:
	std::string name;
	int swizzle;
	Color color;
	
	std::set<const Government *> allies;
	std::set<const Government *> enemies;
	
	mutable std::map<const Government *, double> provoked;
};



#endif
