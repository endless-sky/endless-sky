/* FleetAttractionToPiratesCalculator.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Ship.h"
#include "FleetAttractionToPiratesCalculator.h"
#include "GameData.h"
#include "GameParameters.h"
#include "Messages.h"

#include <limits>

using namespace std;



FleetAttractionToPiratesCalculator::FleetAttractionToPiratesCalculator(const std::vector<std::shared_ptr<Ship>> &ships)
	: totalDamagePerSecond(0)
	, totalHullRepairRate(0)
	, totalShieldGeneration(0)
	, totalOutfitValue(0)
	, totalCargoTonnage(0)
	, attractionFactor(0)
{
	// don't pick on new players.
	// if the total outfit space is less than a threshold and the player only owns one ship don't spawn any additional pirate raids
	double totalOutfitSpace = 0;

	for(const shared_ptr<Ship> &ship : ships)
	{
		totalOutfitSpace += ship->Attributes().Get("outfit space");
		totalCargoTonnage += ship->Cargo().Used();

		for(auto &it : ship->Outfits())
		{
			if(ship->IsParked())
				continue;

			const Outfit *outfit = it.first;
			const int count = it.second;

			totalOutfitValue += outfit->Cost() * count;

			if(outfit->IsWeapon())
			{
				double dps = outfit->ShieldDamagePerSecond()
					+ outfit->HullDamagePerSecond()
					+ outfit->HeatDamagePerSecond()
					+ outfit->IonDamagePerSecond()
					+ outfit->DisruptionDamagePerSecond();

				totalDamagePerSecond += dps * count;
			}
			else
			{
				totalHullRepairRate += outfit->Get("hull repair rate") * count;
				totalShieldGeneration += outfit->Get("shield generation") * count;
			}
		}
	}

	if(totalOutfitSpace > GameData::Parameters().PirateAttractionMinimumOutfitSpace())
	{
		double damagePerSecondFactor = totalDamagePerSecond / GameData::Parameters().PirateAttractionFactorDamagePerSecond();
		double hullRepairRateFactor = totalHullRepairRate / GameData::Parameters().PirateAttractionFactorHullRepairRate();
		double shieldGenerationFactor = totalShieldGeneration / GameData::Parameters().PirateAttractionFactorShieldGeneration();
		double outfitValueFactor = totalOutfitValue / GameData::Parameters().PirateAttractionFactorOutfitValue();
		double cargoTonnageFactor = totalCargoTonnage / GameData::Parameters().PirateAttractionFactorCargoTonnage();

		double temp = damagePerSecondFactor + hullRepairRateFactor + shieldGenerationFactor + outfitValueFactor + cargoTonnageFactor + .5;

		if(temp <= 0)
			attractionFactor = 0;
		else if(temp >= numeric_limits<unsigned int>::max())
			attractionFactor = numeric_limits<unsigned int>::max();
		else
			attractionFactor = temp;

		Messages::Add("dps:" + to_string(damagePerSecondFactor) + " sh:" + to_string(shieldGenerationFactor) + " $:" +
			to_string(outfitValueFactor) + " car:" + to_string(cargoTonnageFactor) + " = " + to_string(temp));
	}
}

FleetAttractionToPiratesCalculator::~FleetAttractionToPiratesCalculator()
{
}

