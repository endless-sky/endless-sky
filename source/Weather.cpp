/* Weather.cpp
Copyright (c) 2020 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Weather.h"

#include "Angle.h"
#include "Hazard.h"
#include "Visual.h"
#include "Random.h"

#include <cmath>

using namespace std;

Weather::Weather(const Hazard *hazard, int totalLifetime, int lifetimeRemaining, double strength)
	: hazard(hazard), totalLifetime(totalLifetime), lifetimeRemaining(lifetimeRemaining), strength(strength)
{
	// Using a deviation of totalLifetime / 4.3 causes the strength of the
	// weather to start and end at about 10% the maximum. Store the entire
	// denominator of the exponent for the normal curve euqation here since
	// this doesn't change with the elapsed time.
	deviation = totalLifetime / 4.3;
	deviation = 2. * deviation * deviation;
	currentStrength = strength;
}



// The hazard that is associated with this weather event.
const Hazard *Weather::GetHazard() const
{
	return hazard;
}



// Whether the hazard of this weather deals damage or not.
bool Weather::HasWeapon() const
{
	return hazard->IsWeapon();
}



// The period of this weather, dictating how often it deals damage while active.
int Weather::Period() const
{
	// If a hazard deviates, then the period is divided by the square root of the
	// strength. This is so that as the strength of a hazard increases, it gets both
	// more likely to impact the ships in the system and each impact hits harder.
	return hazard->Deviates() ? max(1, static_cast<int>(hazard->Period() / sqrtStrength)) : hazard->Period();
}



// What the hazard's damage is multiplied by given the current weather strength.
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
		double truePeriod = hazard->Period() / sqrtStrength;
		double multiplier = max(1, static_cast<int>(truePeriod)) / truePeriod;
		return sqrtStrength * multiplier;
	}
	else
		return currentStrength;
}



// Create any environmental effects and decrease the lifetime of this weather.
void Weather::Step(vector<Visual> &visuals)
{
	// Environmental effects are created by choosing a random angle and distance from
	// the system center, then creating the effect there.
	double minRange = hazard->MinRange();
	double maxRange = hazard->MaxRange();
	
	// Estimate the number of visuals to be generated this frame.
	// MAYBE: create only a subset of possible effects per frame.
	int totalAmount = 0;
	for(auto &&effect : hazard->EnvironmentalEffects())
		totalAmount += effect.second;
	totalAmount *= currentStrength;
	visuals.reserve(visuals.size() + totalAmount);
	
	for(auto &&effect : hazard->EnvironmentalEffects())
		for(int i = 0; i < effect.second * currentStrength; ++i)
		{
			Point angle = Angle::Random().Unit();
			double magnitude = (maxRange - minRange) * sqrt(Random::Real());
			Point pos = (minRange + magnitude) * angle;
			visuals.emplace_back(*effect.first, std::move(pos), Point(), Angle::Random());
		}
	
	if(--lifetimeRemaining <= 0)
		shouldBeRemoved = true;
}



// Calculate this weather's strength for the current frame, to be used to find
// out what the current period and damage multipliers are.
void Weather::CalculateStrength()
{
	// If this hazard deviates, modulate strength by the current lifetime.
	// Strength follows a normal curve, peaking when the lifetime has
	// reached half the totalLifetime.
	if(hazard->Deviates())
	{
		double offset = lifetimeRemaining - totalLifetime / 2.;
		currentStrength = strength * exp(-offset * offset / deviation);
		sqrtStrength = sqrt(currentStrength);
	}
}



// Check if this object is marked for removal from the game.
bool Weather::ShouldBeRemoved() const
{
	return shouldBeRemoved;
}
