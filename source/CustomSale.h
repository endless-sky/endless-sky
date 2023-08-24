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

#ifndef CUSTOM_SALE_H_
#define CUSTOM_SALE_H_

#include "ConditionSet.h"
#include "LocationFilter.h"
#include "Sale.h"
#include "Set.h"

#include <functional>
#include <map>
#include <set>
#include <string>

class ConditionsStore;
class DataNode;
class Planet;
class Outfit;



// Class used to stock Outfits and their local changes, being prices and sell types,
// linked by outfit or by group of outfits (aka outfitters).
class CustomSale {
public:
	// Sell types: none is meant to be default, meaning the visibility depends on the outfitter,
	// import means it is shown whilst still not being buyable.
	//
	// The numbers correspond to the priority, import will override the default.
	enum class SellType {
		DEFAULT = 0,
		IMPORT = 1
	};


public:
	// If the changes are events that means the outfits have already been loaded
	// and we can call FinishLoading() straight away.
	void Load(const DataNode &node, bool eventChange = false);
	void FinishLoading();

	// Adds another CustomSale to this one if the conditions allow it.
	bool Add(const CustomSale &other, const Planet &planet, const ConditionsStore &store);

	// Get the price of the item.
	// Does not check conditions are met or the location is matched.
	double GetRelativeCost(const Outfit &item) const;

	SellType GetSellType() const;

	// Convert the given sellType into a string.
	static const std::string &GetShown(SellType sellType);

	bool Has(const Outfit &item) const;

	// Check if this planet with the given conditions of the player match the conditions of the CustomSale.
	bool Matches(const Planet &planet, const ConditionsStore &playerConditions) const;

	bool IsEmpty();


private:
	void Clear();


private:
	std::string name;
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

	// When loading we cannot be sure all outfits are loaded, so store those we need to convert into relative values.
	std::vector<std::pair<const Outfit *, double *>> toConvert;
};

#endif

