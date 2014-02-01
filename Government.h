/* Government.h
Michael Zahniser, 31 Jan 2014

Class representing a government. All ships have a government, and each
government can have others as allies (those it will assist if they are under
attack) or enemies (those it will attack without provocation). These relations
need not be mutual. For example, pirates attack merchants, but merchants do not
seek out fights with pirates; Republic warships defend merchants who are under
attack, but merchants do not join in when Republic ships are fighting against
Free Worlds ships.
*/

#ifndef GOVERNMENT_H_INCLUDED
#define GOVERNMENT_H_INCLUDED

#include "DataFile.h"
#include "Set.h"

#include <set>
#include <string>



class Government {
public:
	// Default constructor.
	Government();
	
	// Load a government's definition from a file.
	void Load(const DataFile::Node &node, const Set<Government> &others);
	
	// Get the name of this government.
	const std::string &GetName() const;
	// Get the color swizzle to use for ships of this government.
	int GetColor() const;
	
	// Check whether ships of this government will come to the aid of ships of
	// the given government that are under attack.
	bool IsAlly(const Government *other) const;
	// Check whether ships of this government will preemptively attack ships of
	// the given government.
	bool IsEnemy(const Government *other) const;
	
	// Mark that this government is, for the moment, fighting the given
	// government, which is not necessarily one of its normal enemies, because
	// a ship of that government attacked it or one of its allies.
	void Provoke(const Government *other);
	// Reset the record of who has provoked whom. Typically this will happen
	// whenever you move to a new system.
	void ResetProvocation();
	
	
private:
	std::string name;
	int color;
	
	std::set<const Government *> allies;
	std::set<const Government *> enemies;
	
	std::set<const Government *> provoked;
};



#endif
