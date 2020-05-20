/* Weather.cpp
Copyright (c) 2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Weather.h"

#include <cmath>

using namespace std;

Weather::Weather(const Hazard *hazard, int lifetime, double strength, int totalLifetime)
	: hazard(hazard), totalLifetime((totalLifetime > 0) ? totalLifetime : lifetime), lifetime(lifetime), strength(strength)
{
}



const Hazard *Weather::GetHazard() const
{
	return hazard;
}



bool Weather::HasWeapon() const
{
	return hazard->IsWeapon();
}



int Weather::Period() const
{
	// If a hazard deviates, then the period is divided by the square root of the
	// strength. This is so that as the strength of a hazard increases, it gets both
	// more likely to impact the ships in the system and each impact hits harder.
	if(hazard->Deviates())
		return max(1, static_cast<int>(static_cast<double>(hazard->Period()) / sqrt(Strength())));
	else
		return hazard->Period();
}



double Weather::DamageMultiplier() const
{
	// If a hazard deviates, then the damage is multiplied by the square root of the
	// strength. This is so that as the strength of a hazard increases, it gets both
	// more likely to impact the ships in the system and each impact hits harder.
	if(hazard->Deviates())
	{
		// If the square root of the strength is greater than the period, then Period()
		// will return 1. Given this, we need to multiply the amount of strength
		// going toward the damage by some corrective factor. Figure out what the "true
		// period" is (without it bottoming out at 1) and divide that with the current
		// period in order to correctly scale the damage so that the DPS of the hazard
		// will always scale properly with the strength.
		// This also fixes some precision lost by the fact that the period is an integer.
		double truePeriod = static_cast<double>(hazard->Period()) / sqrt(Strength());
		double multiplier = static_cast<double>(Period()) / truePeriod;
		return sqrt(Strength()) * multiplier;
	}
	else
		return strength;
}



int Weather::Step(vector<Visual> &newVisuals)
{
	// Create the environmental effects... somehow.
	return --lifetime;
}



double Weather::Strength() const
{
	if(hazard->Deviates())
	{
		double castTotal = static_cast<double>(totalLifetime);
		double castCurrent = static_cast<double>(lifetime);
		// Using a deviation of totalLifetime / 4.3 causes the strength of the weather to start and end at about 10% the maximum.
		double DEVIATION = castTotal / 4.3;
		// Modulate strength by the current lifetime. Strength follows a normal curve, peaking when the lifetime has reached half the totalLifetime.
		return strength * exp(-pow(castCurrent - castTotal / 2., 2.) / (2. * pow(DEVIATION, 2.)));
	}
	else
		return strength;
}
