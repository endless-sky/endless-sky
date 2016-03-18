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
private:
	typedef std::map<int, int> InnerMap;
	typedef std::map<const Outfit*, InnerMap> OuterMap;
	
public:
	static int64_t CostFunction(const Outfit *outfit, int age, double minValue = 0.5, double lossPerDay = 0.0025);
	static double CostFunction(int age, double minValue = 0.5, double lossPerDay = 0.0025);
	static int UsedAge(double minValue = 0.5, double lossPerDay = 0.0025);
	static int PlunderAge(double minValue = 0.5, double lossPerDay = 0.0025);

public:
	OutfitGroup() { outfits.empty(); };
	
	void Clear();
	bool Empty() const;
	const std::map<int, int> *Find(const Outfit *outfit) const;
	
	// Given attribute summed over all outfits in group.
	double GetTotalAttribute(std::string attribute) const;
	// Cost of all outfits in group.
	int64_t GetTotalCost() const;
	// Cost of all outfits of a given type.
	int64_t GetTotalCost(const Outfit *outfit) const;
	// How many outfits of a given type.
	int GetTotalCount(const Outfit *outfit) const;
	// Cost of given number of outfits of a given type, either oldest or newest. 
	int64_t GetCost(const Outfit* outfit, int count, bool oldestFirst) const;
	
	// Add outfits.
	int AddOutfit(const Outfit* outfit, int count, int age);
	
	// Remove outfits, either oldest first or newest first. Return number removed.
	int RemoveOutfit(const Outfit* outfit, int count, bool oldestFirst, OutfitGroup* to = nullptr);
	
	int TransferOutfits(const Outfit *outfit, int count, OutfitGroup* to, bool oldestFirst, int defaultAge = 0);
	
	void IncrementDate(int days = 1);
	
	// Iterator used to let OutfitGroup be used in for loops just like 
	// it was a map<const Outfit *, int>.  
	class iterator {
	public:
		iterator (const OutfitGroup* group, bool begin);
		iterator (const OutfitGroup* group, const Outfit* outfit);
		
		// Iteration operators
		bool operator!= (const OutfitGroup::iterator& other) const;
		bool operator== (const OutfitGroup::iterator& other) const;
		iterator operator* () const;
		const OutfitGroup::iterator& operator++ ();
				
		// Getters 
		const Outfit* GetOutfit() const;
		int GetAge() const;
		int GetQuantity() const;
		int64_t GetTotalCost() const;
		double GetCostRatio()const;
		std::string GetCostRatioString() const;

		
	private:
		const OutfitGroup *myGroup;
		OuterMap::const_iterator outerIter;
		InnerMap::const_iterator innerIter;
		bool isEnd;
	};

	
	// Enable use in for loops.
	OutfitGroup::iterator begin() const;
	OutfitGroup::iterator end() const;
	OutfitGroup::iterator find(const Outfit *outfit) const;
	
	
private:
	// Map of outfit to map of ages to outfits of that type of that age.
	OuterMap outfits;

};



#endif
