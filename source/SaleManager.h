/* SaleManager.h
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

#pragma once

#include "Stock.h"

#include <memory>
#include <vector>

class Depreciation;
class Outfit;
class PlayerInfo;
class Ship;



// A class for managing the buying and selling of items in a shop.
class SaleManager {
public:
	SaleManager(const PlayerInfo &player, const Stock<Outfit> *outfitter = nullptr,
		const Stock<Ship> *shipyard = nullptr);

	// Whether the given item is available in the shops tracked by this SaleManager.
	// Currently only accounts for permanent stock. That is, items not for sale in
	// the shop and only present because the player sold them are not accounted
	// for by these functions.
	bool Has(const Outfit *outfit) const;
	bool Has(const Ship *ship) const;

	// The buy value of an outfit in the outfitter.
	int64_t BuyValue(const Outfit *outfit, int count = 1) const;
	// The sell value of an outfit in the outfitter. May equal the buy value if the
	// shop has no modifiers, or the outfit was just purchased and is being sold back
	// to the shop at its full price.
	int64_t SellValue(const Outfit *outfit, int count = 1) const;

	// The buy value of a ship in the shipyard.
	int64_t BuyValue(const Ship *ship, int count = 1, bool chassisOnly = false) const;
	// The sell value of a ship in the shipyard. May equal the buy value if the
	// shop has no modifiers, or the ship was just purchased and is being sold back
	// to the shop at its full price. This is only the value of the chassis.
	int64_t SellValue(const Ship *ship, int count = 1) const;

	// The sell value of one of the player's ships, chassis and outfits included.
	int64_t SellValue(const Ship &ship) const;
	// The sell value from a collection of the player's ships.
	int64_t SellValue(const std::vector<std::shared_ptr<Ship>> &fleet, bool chassisOnly = false) const;


private:
	const PlayerInfo &player;
	const Stock<Outfit> *outfitter = nullptr;
	const Stock<Ship> *shipyard = nullptr;

	// The current number of days since the epoch, for determining depreciation.
	int day;
	const Depreciation &fleetDepreciation;
	const Depreciation &stockDepreciation;
};
