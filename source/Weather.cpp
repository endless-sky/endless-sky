/* Weather.cpp
Copyright (c) 2020 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Weather.h"

#include "Angle.h"
#include "Hazard.h"
#include "Random.h"
#include "Screen.h"
#include "Visual.h"

#include <cmath>

using namespace std;

Weather::Weather(const Hazard *hazard, int totalLifetime, int lifetimeRemaining, double strength, Point origin)
	: hazard(hazard), totalLifetime(totalLifetime), lifetimeRemaining(lifetimeRemaining),
		strength(strength), origin(origin)
{
	// Using a deviation of totalLifetime / 4.3 causes the strength of the
	// weather to start and end at about 10% the maximum. Store the entire
	// denominator of the exponent for the normal curve equation here since
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
	return hazard->Weapon::IsLoaded();
}



// The period of this weather, dictating how often it deals damage while active.
int Weather::Period() const
{
	// If a hazard deviates, then the period is divided by the square root of the
	// strength. This is so that as the strength of a hazard increases, it gets both
	// more likely to impact the ships in the system and each impact hits harder.
	return hazard->Deviates() ? max(1, static_cast<int>(hazard->Period() / sqrtStrength)) : hazard->Period();
}



const Point &Weather::Origin() const
{
	return origin;
}



// Create any environmental effects and decrease the lifetime of this weather.
void Weather::Step(vector<Visual> &visuals, const Point &center)
{
	// Environmental effects are created by choosing a random angle and distance from
	// their origin, then creating the effect there.
	double minRange = hazard->MinRange();
	double maxRange = hazard->MaxRange();
	double effectMultiplier = currentStrength;

	// If a hazard is system-wide, the max range becomes the edge of the screen,
	// and the number of effects drawn is scaled accordingly.
	if(hazard->SystemWide() && maxRange > 0.)
	{
		// Find the farthest possible point from the screen center and use that as
		// our new max range. Multiply by 2 to account for the max view zoom level.
		double newMax = 2. * Screen::Dimensions().Length();
		// Maintain the same density of effects by dividing the new area
		// by the old. (The pis cancel out and therefore need not be taken
		// into account.)
		effectMultiplier *= (newMax * newMax) / (maxRange * maxRange);
		maxRange = newMax;
	}

	// Don't draw effects if a system-wide hazard moved the max range to
	// be less than the min range.
	if(minRange <= maxRange)
	{
		// Estimate the number of visuals to be generated this frame.
		// MAYBE: create only a subset of possible effects per frame.
		float totalAmount = 0;
		for(auto &&effect : hazard->EnvironmentalEffects())
			totalAmount += effect.second;
		totalAmount *= effectMultiplier;
		visuals.reserve(visuals.size() + static_cast<int>(totalAmount));

		for(auto &&effect : hazard->EnvironmentalEffects())
			for(int i = 0; i < effect.second * effectMultiplier; ++i)
			{
				Point angle = Angle::Random().Unit();
				double magnitude = (maxRange - minRange) * sqrt(Random::Real());
				Point pos = (hazard->SystemWide() ? center : origin)
					+ (minRange + magnitude) * angle;
				visuals.emplace_back(*effect.first, std::move(pos), Point(), Angle::Random());
			}
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



// Get information on how this hazard impacted a ship.
Weather::ImpactInfo Weather::GetInfo() const
{
	return ImpactInfo(*hazard, origin, DamageMultiplier());
}



// Check if this object is marked for removal from the game.
bool Weather::ShouldBeRemoved() const
{
	return shouldBeRemoved;
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
