/* CustomSaleManager.h
Copyright (c) 2023 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CUSTOM_SALE_MANAGER_H_
#define CUSTOM_SALE_MANAGER_H_

#include "CustomSale.h"

#include <map>

class ConditionsStore;
class Outfit;
class Planet;
class Ship;
class System;



class CustomSaleManager {
public:
	// Refresh the active CustomSales to correspond to the planet and conditions.
	static void Refresh(const Planet *planet, const ConditionsStore &conditions);
	// Or to all planets in this system.
	static void Refresh(const System *system, const ConditionsStore &conditions);
	// Return if can be bought. And imported item will still be shown but not be buyable.
	static bool CanBuy(const Outfit &outfit);

	// Get the full/relative cost of the outfit/ship with the last cached conditions.
	static int64_t OutfitCost(const Outfit &outfit);
	static double OutfitRelativeCost(const Outfit &outfit);
	static int64_t ShipCost(const Ship &ship);
	// Delete all cached data.
	static void Clear();


private:
	static std::map<CustomSale::SellType, CustomSale> customSales;
};

#endif

