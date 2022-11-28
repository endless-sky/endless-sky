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
	// Sell types: none is meant to be default, signifying this is empty,
	// visible is the default one in the game,
	// import means you can only sell it but not buy it (and it is shown still)
	// hidden means it will not be shown except if you have the outfit (and it will have a custom price still).
	// The numbers correspond to the priority, hidden will override import which will override visible.
	enum class SellType {
		NONE = 0,
		VISIBLE = 1,
		IMPORT = 2,
		HIDDEN = 3
	};


public:
	void Load(const DataNode &node, const Set<Sale<Outfit>> &items,
		const Set<Outfit> &outfits, const std::string &mode);

	// Adds another CustomSale of the same sellType to this one, or to any type if this one is empty.
	bool Add(const CustomSale &other);

	// Get the price of the item. One should check if the conditions match first.
	double GetRelativeCost(const Outfit &item) const;

	SellType GetSellType() const;

	// Convert the given sellType into a string.
	static const std::string &GetShown(SellType sellType);

	// Return all outfits that are affected by this CustomSale.
	const Sale<Outfit &> GetOutfits() const;

	bool Has(const Outfit &item) const;

	// Check if this planet with the given conditions of the player match the conditions of the CustomSale.
	bool Matches(const Planet &planet, const ConditionsStore &playerConditions) const;

	void Clear();

	void CheckIfEmpty();

	bool IsEmpty();


private:
	LocationFilter locationFilter;
	ConditionSet conditions;
	const Planet *location = nullptr;

	std::map<const Sale<Outfit> *, double> relativePrices;
	std::map<const Sale<Outfit> *, double> relativeOffsets;

	std::map<const Outfit *, double> relativeOutfitPrices;
	std::map<const Outfit *, double> relativeOutfitOffsets;

	SellType sellType = SellType::NONE;
};

#endif

