/* CustomSale.h
Copyright (c) 2021 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CustomSale_H_
#define CustomSale_H_

#include "ConditionSet.h"
#include "LocationFilter.h"
#include "Outfit.h"
#include "Set.h"
#include "Sale.h"

#include <map>
#include <set>

class DataNode;
class LocationFilter;
class Planet;



// Class used to stock Outfits and their local changes, being prices and sell types, linked by outfit or by group of outfits.
class CustomSale {
public:
	enum class SellType {
		NONE = 0,
		VISIBLE = (1 << 0),
		IMPORT = (1 << 1),
		HIDDEN = (1 << 2)
	};


public:
	void Load(const DataNode &node, const Set<Sale<Outfit>> &items, const Set<Outfit> &outfits);
	
	bool Add(const CustomSale &other);
	
	double GetRelativeCost(const Outfit *item) const;
	
	SellType GetSellType() const;

	static const std::string &GetShown(SellType sellType);
	
	const Sale<Outfit> &GetOutfits() const;
	
	bool Has(const Outfit *item) const;

	bool Matches(const Planet *planet, const ConditionSet::Conditions conditions) const;

	void clear();

	
private:
	mutable Sale<Outfit> seen;
	
	LocationFilter locationFilter;
	ConditionSet toApply;
	const Planet *source = nullptr;
	
	std::map<const Sale<Outfit> *, double> relativePrices;
	std::map<const Sale<Outfit> *, double> relativeOffsets;
	
	std::map<const Outfit *, double> relativeOutfitPrices;
	std::map<const Outfit *, double> relativeOutfitOffsets;
	
	SellType sellType = SellType::NONE;
};

#endif

