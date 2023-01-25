/* CustomSaleManager.cpp
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

#include "CustomSaleManager.h"

#include "ConditionsStore.h"
#include "GameData.h"
#include "Outfit.h"
#include "Planet.h"
#include "Ship.h"
#include "StellarObject.h"
#include "System.h"

using namespace std;

map<CustomSale::SellType, CustomSale> CustomSaleManager::customSales = {};



void CustomSaleManager::Refresh(const Planet *planet, const ConditionsStore &conditions)
{
	Clear();
	if(!planet)
		return;
	for(const auto &sale : GameData::CustomSales())
		customSales[sale.second.GetSellType()].Add(sale.second, *planet, conditions);
}



void CustomSaleManager::Refresh(const System *system, const ConditionsStore &conditions)
{
	Clear();
	if(!system)
		return;
	for(const StellarObject &object : system->Objects())
		if(object.HasSprite() && object.HasValidPlanet())
		{
			const Planet &planet = *object.GetPlanet();
			for(const auto &sale : GameData::CustomSales())
				customSales[sale.second.GetSellType()].Add(sale.second, planet, conditions);
		}
}



bool CustomSaleManager::CanBuy(const Outfit &outfit)
{
	const auto &it = customSales.find(CustomSale::SellType::IMPORT);
	return it == customSales.end() || !it->second.Has(outfit);
}



int64_t CustomSaleManager::OutfitCost(const Outfit &outfit)
{
	return OutfitRelativeCost(outfit) * outfit.Cost();
}



double CustomSaleManager::OutfitRelativeCost(const Outfit &outfit)
{
	if(customSales.empty())
		return 1.;
	// Iterate in the opposite order, since any higher customSale has priority.
	for(auto &&selling = customSales.rbegin(); selling != customSales.rend(); ++selling)
		if(selling->second.Has(outfit))
			return selling->second.GetRelativeCost(outfit);
	return 1.;
}



int64_t CustomSaleManager::ShipCost(const Ship &ship)
{
	int64_t localCost = ship.ChassisCost();
	for(auto &&it : ship.Outfits())
		localCost += OutfitCost(*it.first) * it.second;
	return localCost;
}



void CustomSaleManager::Clear()
{
	customSales = {};
}
