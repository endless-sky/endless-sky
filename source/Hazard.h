/* Hazard.h
Copyright (c) 2020 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef HAZARD_H_
#define HAZARD_H_

#include "Weapon.h"


// Hazards are environmental effects created within systems. They are able to create
// visual effects and damage or apply status effects to any ships within the system
// while active.
class Hazard : public Weapon {
public:
	void Load(const DataNode &node);
	
	const std::string &Name() const;
	// Does the strength of this hazard deviate over time?
	bool Deviates() const;
	// How often this hazard deals its damage while active.
	int Period() const;
	// Generates a random integer between the minimum and maximum duration of this hazard.
	int RandomDuration() const;
	// Generates a random double between the minimum and maximum strength of this hazard.
	double RandomStrength() const;
	// The minimum and maximum distances from the origin in which this hazard has an effect.
	double MinRange() const;
	double MaxRange() const;
	
	// Visuals to be created while this hazard is active.
	const std::map<const Effect *, int> &EnvironmentalEffects() const;
	
	
private:
	std::string name;
	int period = 1;
	int minDuration = 1;
	int maxDuration = 1;
	double minStrength = 1.;
	double maxStrength = 1.;
	double minRange = 0.;
	// Hazards given no range only extend out to the invisible fence defined in AI.cpp.
	double maxRange = 10000.;
	bool deviates = true;
	
	std::map<const Effect *, int> environmentalEffects;
};

#endif
