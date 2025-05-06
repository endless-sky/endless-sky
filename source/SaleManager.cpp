/* SaleManager.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SaleManager.h"

#include "Date.h"
#include "Depreciation.h"
#include "GameData.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "Shop.h"
#include "ShopPricing.h"
#include "StockItem.h"

using namespace std;



SaleManager::SaleManager(const PlayerInfo &player, const Stock<Outfit> *outfitter, const Stock<Ship> *shipyard)
	: player(player), outfitter(outfitter), shipyard(shipyard), day(player.GetDate().DaysSinceEpoch()),
	fleetDepreciation(player.FleetDepreciation()), stockDepreciation(player.StockDepreciation())
{
}



int64_t SaleManager::BuyValue(const Outfit *outfit, int count) const
{
	if(!outfit || count <= 0)
		return 0;

	int64_t baseCost = outfit->Cost();
	if(outfitter)
	{
		// If this item is for sale from the outfitter, use the outfitter's price modifier.
		const StockItem<Outfit> *stock = outfitter->Get(outfit);
		if(stock)
		{
			int64_t value = 0;
			// If an item was just sold, we want to buy it at the same value it was sold
			// at. Determine how many of this item being bought are old, and for those old items,
			// use the selling price. For all remaining items, use the buying price.
			int oldCount = stockDepreciation.NumberOld(outfit, day, count);
			if(oldCount)
			{
				count -= oldCount;
				double fraction = stockDepreciation.ValueFraction(outfit, day, oldCount);
				const ShopPricing &modifier = stock->SellModifier();
				value += modifier.Value(baseCost, oldCount, fraction);
			}
			if(count)
			{
				const ShopPricing &modifier = stock->BuyModifier();
				value += modifier.Value(baseCost, count, count);
			}
			return value;
		}
	}

	// Otherwise, just return the outfit cost with normal depreciation applied.
	double fraction = stockDepreciation.ValueFraction(outfit, day, count);
	return baseCost * fraction;
}



int64_t SaleManager::SellValue(const Outfit *outfit, int count) const
{
	if(!outfit || count <= 0)
		return 0;

	int64_t baseCost = outfit->Cost();

	if(outfitter)
	{
		// If this item is for sale from the outfitter, use the outfitter's price modifier.
		const StockItem<Outfit> *stock = outfitter->Get(outfit);
		if(stock)
		{
			int64_t value = 0;
			// If an item was just bought, we want to sell it at the same value it was bought
			// at. Determine how many of this item being sold are new, and for those new items,
			// use the buying price. For all remaining items, use the selling price.
			int newCount = fleetDepreciation.NumberNew(outfit, day, count);
			if(newCount)
			{
				count -= newCount;
				const ShopPricing &modifier = stock->BuyModifier();
				value += modifier.Value(baseCost, newCount, newCount);
			}
			if(count)
			{
				double fraction = fleetDepreciation.ValueFraction(outfit, day, count);
				const ShopPricing &modifier = stock->SellModifier();
				value += modifier.Value(baseCost, count, fraction);
			}
			return value;
		}
	}

	// Otherwise, just return the base cost with normal depreciation applied.
	double fraction = fleetDepreciation.ValueFraction(outfit, day, count);
	return baseCost * fraction;
}



int64_t SaleManager::BuyValue(const Ship *ship, int count, bool chassisOnly) const
{
	if(!ship || count <= 0)
		return 0;

	ship = GameData::Ships().Get(ship->TrueModelName());
	int64_t baseCost = ship->ChassisCost();

	int64_t value = 0;
	const StockItem<Ship> *stock = nullptr;
	if(shipyard)
	{
		// If this item is for sale from the shipyard, use the shipyard's price modifier.
		// Note: The shipyard should always have a StockItem for the given ship, at least
		// unless we make it so that selling a ship stores its chassis in the shipyard.
		stock = shipyard->Get(ship);
		if(stock)
		{
			// If an item was just sold, we want to buy it at the same value it was sold
			// at. Determine how many of this item being bought are old, and for those old items,
			// use the selling price. For all remaining items, use the buying price.
			int oldCount = stockDepreciation.NumberOld(ship, day, count);
			if(oldCount)
			{
				count -= oldCount;
				double fraction = stockDepreciation.ValueFraction(ship, day, oldCount);
				const ShopPricing &modifier = stock->SellModifier();
				value += modifier.Value(baseCost, oldCount, fraction);
			}
			if(count)
			{
				const ShopPricing &modifier = stock->BuyModifier();
				value += modifier.Value(baseCost, count, count);
			}
		}
	}
	if(!shipyard || !stock)
	{
		// Otherwise, just use the ship cost with normal depreciation applied.
		double fraction = stockDepreciation.ValueFraction(ship, day, count);
		value += baseCost * fraction;
	}

	if(!chassisOnly)
		for(const auto &it : ship->Outfits())
			value += BuyValue(it.first, it.second);
	return value;
}



int64_t SaleManager::SellValue(const Ship *ship, int count) const
{
	if(!ship || count <= 0)
		return 0;

	ship = GameData::Ships().Get(ship->TrueModelName());
	int64_t baseCost = ship->ChassisCost();

	if(shipyard)
	{
		// If this item is for sale from the shipyard, use the shipyard's price modifier.
		const StockItem<Ship> *stock = shipyard->Get(ship);
		if(stock)
		{
			int64_t value = 0;
			// If an item was just bought, we want to sell it at the same value it was bought
			// at. Determine how many of this item being sold are new, and for those new items,
			// use the buying price. For all remaining items, use the selling price.
			int newCount = fleetDepreciation.NumberNew(ship, day, count);
			if(newCount)
			{
				count -= newCount;
				const ShopPricing &modifier = stock->BuyModifier();
				value += modifier.Value(baseCost, newCount, newCount);
			}
			if(count)
			{
				double fraction = fleetDepreciation.ValueFraction(ship, day, count);
				const ShopPricing &modifier = stock->SellModifier();
				value += modifier.Value(baseCost, count, fraction);
			}
			return value;
		}
	}

	// Otherwise, just return the base cost with normal depreciation applied.
	double fraction = fleetDepreciation.ValueFraction(ship, day, count);
	return baseCost * fraction;
}



int64_t SaleManager::SellValue(const Ship &ship) const
{
	int64_t value = SellValue(&ship);
	for(const auto &it : ship.Outfits())
		value += SellValue(it.first, it.second);
	return value;
}



int64_t SaleManager::SellValue(const vector<shared_ptr<Ship>> &fleet, bool chassisOnly) const
{
	if(fleet.empty())
		return 0;

	// Determine how many of each ship and outfit is being sold.
	map<const Ship *, int> shipCount;
	map<const Outfit *, int> outfitCount;

	for(const shared_ptr<Ship> &ship : fleet)
	{
		const Ship *base = GameData::Ships().Get(ship->TrueModelName());
		++shipCount[base];

		if(!chassisOnly)
			for(const auto &it : ship->Outfits())
				outfitCount[it.first] += it.second;
	}

	// Add up the value across all ship chassis and outfits being sold.
	int64_t value = 0;
	for(const auto &it : shipCount)
		value += SellValue(it.first, it.second);
	for(const auto &it : outfitCount)
		value += SellValue(it.first, it.second);
	return value;
}
