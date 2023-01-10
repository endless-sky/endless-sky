/* CustomSale.h
Copyright (c) 2021 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CustomSale_H_
#define CustomSale_H_

#include "ConditionSet.h"
#include "Outfit.h"
#include "LocationFilter.h"
#include "Sale.h"
#include "Set.h"

#include <map>
#include <set>

class ConditionsStore;
class DataNode;
class Planet;



// Class used to stock Outfits and their local changes, being prices and sell types,
// linked by outfit or by group of outfits (aka outfitters).
class CustomSale {
public:
	// Sell types: none is meant to be default, meaning the visibility depends on the outfitter,
	// import means it is shown whilst still not being buyable
	//
	// The numbers correspond to the priority, hidden will override import which will override visible.
	enum class SellType {
		DEFAULT = 0,
		IMPORT = 1
	};


public:
	void Load(const DataNode &node, const Set<Sale<Outfit>> &items, const Set<Outfit> &outfits);

	// Adds another CustomSale to this one if the conditions allow it.
	bool Add(const CustomSale &other, const Planet &planet, const ConditionsStore &store);

	// Get the price of the item.
	// Does not check conditions are met or the location is matched.
	double GetRelativeCost(const Outfit &item) const;

	SellType GetSellType() const;

	// Convert the given sellType into a string.
	static const std::string &GetShown(SellType sellType);

	// Return all outfits that are affected by this CustomSale.
	const Sale<Outfit> &GetOutfits();

	bool Has(const Outfit &item) const;

	// Check if this planet with the given conditions of the player match the conditions of the CustomSale.
	bool Matches(const Planet &planet, const ConditionsStore &playerConditions) const;

	bool IsEmpty();


private:
	void Clear();


private:
	LocationFilter locationFilter;
	ConditionSet conditions;
	const Planet *location = nullptr;

	std::map<const Sale<Outfit> *, double> relativePrices;
	std::map<const Sale<Outfit> *, double> relativeOffsets;

	std::map<const Outfit *, double> relativeOutfitPrices;
	std::map<const Outfit *, double> relativeOutfitOffsets;

	// All outfits this customSale has, kept in cache.
	Sale<Outfit> seen;
	bool cacheValid = false;

	SellType sellType = SellType::DEFAULT;
};

#endif

