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

#include "Mask.h"
#include "Ship.h"
#include "Weapon.h"

using namespace std;

HazardProfile::HazardProfile(const Projectile::ImpactInfo &info, double damageScaling, bool isBlast)
	: weapon(info.weapon), position(info.position), inputScaling(damageScaling), isBlast(isBlast)
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



// Calculate the damage dealt to the given ship.
DamageProfile::DamageDealt HazardProfile::CalculateDamage(const Ship &ship) const
{
	// Calculate the final damage scale specific to this ship.
	DamageDealt damage(weapon, Scale(inputScaling, ship), isBlast);
	PopulateDamage(damage, ship, position);

	return damage;
}



// Determine the damage scale for the given ship.
double HazardProfile::Scale(double scale, const Ship &ship) const
{
	// Finish the blast radius calculations.
	double distance = -1.;
	if(isBlast && weapon.IsDamageScaled())
	{
		// Rather than exactly compute the distance between the explosion and
		// the closest point on the ship, estimate it using the mask's Radius.
		if(distance < 0.)
			distance = max(0., position.Distance(ship.Position()) - ship.GetMask().Radius());
		double finalR = distance * distance * rSquared;
		scale *= k / ((1. + finalR * finalR) * (1. + finalR * finalR));
	}
	if(weapon.HasDamageDropoff())
	{
		if(distance < 0.)
			distance = max(0., position.Distance(ship.Position()) - ship.GetMask().Radius());
		scale *= weapon.DamageDropoff(distance);
	}
	
	return scale;
}
