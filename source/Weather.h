/* Weather.h
Copyright (c) 2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WEATHER_H_
#define WEATHER_H_

#include "Hazard.h"
#include "Visual.h"

#include <string>
#include <vector>

class Hazard;
class Visual;


// Weather is used to contain an active system hazard, keeping track of the hazard's lifetime, its strength, and if it should cause any damage.
class Weather {
public:
	Weather() = default;
	Weather(const Hazard *harzard, int lifetime, double strength, int totalLifetime = -1);
	
	const Hazard *GetHazard() const;
	// Whether the hazard of this weather deals damage or not.
	bool HasWeapon() const;
	// The probability of this weather dealing damage while active.
	int Period() const;
	// What the hazard's damage is multiplied by given the current strength.
	double DamageMultiplier() const;
	// Create any environmental effects and decrease the lifetime of this weather.
	int Step(std::vector<Visual> &newVisuals);

private:
	// The current strength of this weather, to be used to find out what the
	// current period and damage multipliers are.
	double Strength() const;
	
private:
	const Hazard *hazard = nullptr;
	int totalLifetime;
	int lifetime;
	double strength;
};

#endif
