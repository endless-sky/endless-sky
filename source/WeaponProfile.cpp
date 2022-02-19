/* WeaponProfile.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "WeaponProfile.h"

#include "Mask.h"
#include "Ship.h"
#include "Weapon.h"

using namespace std;

WeaponProfile::WeaponProfile(const Projectile::ImpactInfo &info, bool isBlast)
	: weapon(info.weapon), position(info.position), isBlast(isBlast)
{
	// Calculate the value of certain variables necessary for determining
	// the impact of an explosion that are shared across all ships that
	// this hazard could impact.
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
	// For weapon projectiles, the distance traveled for the projectile
	// is the same regardless of the ship being impacted, so calculate
	// its effect on the damage scale here.
	if(weapon.HasDamageDropoff())
		inputScaling *= weapon.DamageDropoff(info.distanceTraveled);
}



// Set whether blast damage is applied on the next CalculateDamage call.
// When a projectile explodes, if it was caused to explode because it
// directly impacted a ship then the ship that was directly impacted does
// not run the blast radius calculations.
void WeaponProfile::SetBlast(bool blast)
{
	isBlast = blast;
}



// Calculate the damage dealt to the given ship.
DamageProfile::DamageDealt WeaponProfile::CalculateDamage(const Ship &ship) const
{
	DamageDealt damage(weapon, Scale(inputScaling, ship), isBlast);
	PopulateDamage(damage, ship, position);

	return damage;
}



// Determine the damage scale for the given ship.
double WeaponProfile::Scale(double scale, const Ship &ship) const
{
	// Now that we have a specific ship, we can finish the blast damage
	// calculations.
	if(isBlast && weapon.IsDamageScaled())
	{
		// Rather than exactly compute the distance between the explosion and
		// the closest point on the ship, estimate it using the mask's Radius.
		double distance = max(0., position.Distance(ship.Position()) - ship.GetMask().Radius());
		double finalR = distance * distance * rSquared;
		scale *= k / ((1. + finalR * finalR) * (1. + finalR * finalR));
	}

	return scale;
}
