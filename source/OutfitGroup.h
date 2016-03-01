/* OutfitGroup.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef OUTFIT_GROUP_H_
#define OUTFIT_GROUP_H_

#include "Outfit.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class Effect;
class Sound;
class Sprite;
class OutfitGroup;


// Class representing a group of outfits that is installed in a ship 
// or available in a shop. It encapsulates and manages a map of sets which 
// represents not only which outfits are in the group and the age of each 
// outfit in the group for purpose of depreciation. 
// The Map is keyed by pointer to the outfit in question and the list 
// contains the number of outfits of each age, sorted by age, so that 
// it is easy to buy lowest price first or sell highest price first.  
// Replaces std::map<const Outfit *, int> in many places.
class OutfitGroup {
public:
	
	// Save a full description of this outfitGroup and what it currently contains.
	void Save(DataWriter &out) const;
	
	int64_t GetTotalCost() const;
	int64_t GetCost(const Outfit* outfit, int count, bool oldestFirst) const;
	
	// Add outfits.
	void AddOutfit(const Outfit* outfit, int count, int age);
	
	// Remove outfits, either oldest first or newest first.
	void RemoveOutfit(const Outfit* outfit, int count, bool oldestFirst);
	
	void TransferOutfits(const Outfit *outfit, int count, OutfitGroup* to);
	
	void IncrementDate();
	
	// Iterator used to let OutfitGroup be used in for loops just like 
	// it was a map<const Outfit *, int>.  
	class iterator {
	public:
		iterator (const OutfitGroup* group)
		: myGroup(group)
		{ }

		bool operator!= (const OutfitGroup::iterator& other) const;
		int operator* () const;
		const OutfitGroup::iterator& operator++ ();
		
	private:
		const OutfitGroup *myGroup;
		std::map<const Outfit*, std::map<int64_t, int>>::iterator outerIter;
		const std::map<int64_t, int> *currentOutfit;
		std::map<int64_t, int>::iterator innerIter;
	};

	// Enable use in for loops.
	OutfitGroup::iterator begin();
	OutfitGroup::iterator end();
	
private:
	
	// Map of outfit to map of ages to outfits of that type of that age.
	std::map<const Outfit*, std::map<int64_t, int>> outfits;

};



#endif
