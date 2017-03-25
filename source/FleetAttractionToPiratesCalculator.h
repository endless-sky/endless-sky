/* FleetAttractionToPiratesCalculator.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FLEETATTRACTIONTOPIRATESCALCULATOR_H_
#define FLEETATTRACTIONTOPIRATESCALCULATOR_H_

#include <vector>

class Ship;



// wrapper for determining if a pirate raid fleet is attracted to another fleet
class FleetAttractionToPiratesCalculator {
public:
	explicit FleetAttractionToPiratesCalculator(const std::vector<std::shared_ptr<Ship>> &ships);
	~FleetAttractionToPiratesCalculator();
	
	unsigned AttractionFactor() const;

private:
	double totalDamagePerSecond;
	double totalHullRepairRate;
	double totalShieldGeneration;
	double totalOutfitValue;
	double totalCargoTonnage;

	// number between 0 and 200; 0 == no attraction, dont roll for pirates, 1 = 0.5%, 200 == 100%
	unsigned attractionFactor;
};

inline unsigned FleetAttractionToPiratesCalculator::AttractionFactor() const { return attractionFactor; }

#endif
