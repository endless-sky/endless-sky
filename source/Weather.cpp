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



bool Weather::HasWeapon() const
{
	return hazard->IsWeapon();
}



int Weather::Period() const
{
	if(hazard->Deviates())
		return hazard->Period() / GetStrength();
	else
		return hazard->Period();
}



const Hazard *Weather::GetHazard() const
{
	return hazard;
}



double Weather::GetStrength() const
{
	if(hazard->Deviates())
	{
		// Using a deviation of totalLifetime / 4.3 causes the strength of the weather to start and end at about 10% the maximum.
		double DEVIATION = totalLifetime / 4.3;
		// Modulate strength by the current lifetime. Strength follows a normal curve, peaking when the lifetime has reached half the totalLifetime.
		return sqrt(strength * exp(-pow(lifetime - totalLifetime / 2. , 2.) / (2. *pow(DEVIATION , 2.))));
	}
	else
		return strength;
}



int Weather::Step(vector<Visual> &newVisuals)
{
	// Create the environmental effects... somehow.
	return --lifetime;
}
