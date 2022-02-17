/* DamageProfile.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DamageProfile.h"

#include "Mask.h"
#include "Outfit.h"
#include "Point.h"
#include "Ship.h"
#include "Weapon.h"

using namespace std;

DamageProfile::DamageProfile(const Ship &ship, const Weapon &weapon, double damageScaling, double distanceTraveled, const Point &damagePosition, bool isBlast)
	: ship(ship), attributes(ship.Attributes()), weapon(weapon), scaling(damageScaling), 
	distanceTraveled(distanceTraveled), position(damagePosition), isBlast(isBlast)
{
	// Determine if the damage scaling needs changed any further
	// beyond what was already given.
	if(isBlast && weapon.IsDamageScaled())
	{
		// Scale blast damage based on the distance from the blast
		// origin and if the projectile uses a trigger radius. The
		// point of contact must be measured on the sprite outline.
		// scale = (1 + (tr / (2 * br))^2) / (1 + r^4)^2
		double blastRadius = max(1., weapon.BlastRadius());
		double radiusRatio = weapon.TriggerRadius() / blastRadius;
		double k = !radiusRatio ? 1. : (1. + .25 * radiusRatio * radiusRatio);
		// Rather than exactly compute the distance between the explosion and
		// the closest point on the ship, estimate it using the mask's Radius.
		double d = max(0., (position - ship.Position()).Length() - ship.GetMask().Radius());
		double rSquared = d * d / (blastRadius * blastRadius);
		scaling *= k / ((1. + rSquared * rSquared) * (1. + rSquared * rSquared));
	}
	if(weapon.HasDamageDropoff())
		scaling *= weapon.DamageDropoff(distanceTraveled);
}

// Calculate the damage dealt to the ship given its current shield and disruption levels.
void DamageProfile::CalculateDamage(double shields, double disrupted)
{
	shieldFraction = 0.;
	if(shields > 0.)
	{
		double piercing = max(0., min(1., weapon.Piercing() / (1. + attributes.Get("piercing protection")) - attributes.Get("piercing resistance")));
		shieldFraction = (1. - piercing) / (1. + disrupted * .01);

		shieldDamage = (weapon.ShieldDamage()
			+ weapon.RelativeShieldDamage() * attributes.Get("shields"))
			* ScaleType(0., "shield");
		if(shieldDamage > shields)
			shieldFraction = min(shieldFraction, shields / shieldDamage);
	}

	// Instantaneous damage types.
	// Energy, heat, and fuel damage are blocked 50% by shields.
	// Hull damage is blocked 100%.
	// Shield damage is blocked 0%.
	shieldDamage *= shieldFraction;
	hullDamage = (weapon.HullDamage()
		+ weapon.RelativeHullDamage() * attributes.Get("hull"))
		* ScaleType(1., "hull");
	energyDamage = (weapon.EnergyDamage()
		+ weapon.RelativeEnergyDamage() * attributes.Get("energy capacity"))
		* ScaleType(.5, "energy");
	heatDamage = (weapon.HeatDamage()
		+ weapon.RelativeHeatDamage() * ship.MaximumHeat())
		* ScaleType(.5, "heat");
	fuelDamage = (weapon.FuelDamage()
		+ weapon.RelativeFuelDamage() * attributes.Get("fuel capacity"))
		* ScaleType(.5, "fuel");

	// DoT damage types with an instantaneous analog.
	// Ion and burn damage are blocked 50% by shields.
	// Corrosion and leak damage are blocked 100%.
	// Discharge damage is blocked 0%.
	dischargeDamage = weapon.DischargeDamage() * ScaleType(0., "discharge");
	corrosionDamage = weapon.CorrosionDamage() * ScaleType(1., "corrosion");
	ionDamage = weapon.IonDamage() * ScaleType(.5, "ion");
	burnDamage = weapon.BurnDamage() * ScaleType(.5, "burn");
	leakDamage = weapon.LeakDamage() * ScaleType(1., "leak");

	// Unique special damage types.
	// Disruption and slowing are blocked 50% by shields.
	// Hit force is blocked 0%.
	disruptionDamage = weapon.DisruptionDamage() * ScaleType(.5, "disruption");
	slowingDamage = weapon.SlowingDamage() * ScaleType(.5, "slowing");
	hitForce = weapon.HitForce() * ScaleType(0., "force");
}



const Weapon &DamageProfile::GetWeapon() const
{
	return weapon;
}



double DamageProfile::Scaling() const
{
	return scaling;
}



const Point &DamageProfile::Position() const
{
	return position;
}



bool DamageProfile::IsBlast() const
{
	return isBlast;
}



// Return the damage scale that a damage type should use given the
// default percentage that is blocked by shields and the name of
// its protection attribute.
double DamageProfile::ScaleType(double blocked, const string &attr) const
{
	return scaling * (1. - blocked * shieldFraction) / (1. + attributes.Get(attr + " protection"));
}
