/* HazardProfile.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "HazardProfile.h"

using namespace std;

HazardProfile::HazardProfile(const Projectile::ImpactInfo &info, double damageScaling, bool isBlast)
	: weapon(info.weapon), position(info.position), distanceTraveled(info.distanceTraveled),
	inputScaling(damageScaling), isBlast(isBlast)
{
	// Precompute blast damage scaling.
	if(isBlast && weapon.IsDamageScaled())
	{
		// Scale blast damage based on the distance from the blast
		// origin and if the projectile uses a trigger radius. The
		// point of contact must be measured on the sprite outline.
		// scale = (1 + (tr / (2 * br))^2) / (1 + r^4)^2
		double blastRadius = max(1., weapon.BlastRadius());
		double radiusRatio = weapon.TriggerRadius() / blastRadius;
		k = !radiusRatio ? 1. : (1. + .25 * radiusRatio * radiusRatio);
		rSquared = 1. / (blastRadius * blastRadius);
	}
}



// Set a distance traveled to be used on the next CalculateDamage call.
void HazardProfile::SetDistance(double distance)
{
	distanceTraveled = distance;
}



// Calculate the damage dealt to the given ship.
DamageProfile::DamageDealt HazardProfile::CalculateDamage(const Ship &ship) const
{
	// Calculate the final damage scale specific to this ship.
	DamageDealt damage(weapon, inputScaling, isBlast);
	FinishPrecalculations(damage, ship);
	PopulateDamage(damage, ship, position);

	return damage;
}



// Finish any calculations that were started in the constructor.
void HazardProfile::FinishPrecalculations(DamageProfile::DamageDealt &damage, const Ship &ship) const
{
	// Finish the blast radius calculations.
	if(isBlast && weapon.IsDamageScaled())
	{
		// Rather than exactly compute the distance between the explosion and
		// the closest point on the ship, estimate it using the mask's Radius.
		double d = max(0., (position - ship.Position()).Length() - ship.GetMask().Radius());
		double finalR = d * d * rSquared;
		damage.scaling *= k / ((1. + finalR * finalR) * (1. + finalR * finalR));
	}
	if(weapon.HasDamageDropoff())
		damage.scaling *= weapon.DamageDropoff(distanceTraveled);
}
