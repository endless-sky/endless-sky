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
#include "Ship.h"
#include "Weapon.h"

using namespace std;

DamageProfile::DamageProfile(const Projectile::ImpactInfo &info, double damageScaling, bool isBlast, bool skipFalloff)
	: weapon(info.weapon), position(info.position), distanceTraveled(info.distanceTraveled),
	inputScaling(damageScaling), isBlast(isBlast), skipFalloff(skipFalloff)
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
	if(!skipFalloff && weapon.HasDamageDropoff())
		inputScaling *= weapon.DamageDropoff(distanceTraveled);
}



// Set a distance traveled to be used on the next CalculateDamage call,
// assuming skipFalloff is true.
void DamageProfile::SetDistance(double distance)
{
	distanceTraveled = distance;
}



// Set whether blast damage is applied on the next CalculateDamage call.
void DamageProfile::SetBlast(bool blast)
{
	isBlast = blast;
}



// Calculate the damage dealt to the given ship.
DamageProfile::DamageDealt DamageProfile::CalculateDamage(const Ship &ship, double shields, double disrupted) const
{
	const Outfit &attributes = ship.Attributes();

	// Calculate the final damage scale specific to this ship.
	DamageDealt damage(weapon, inputScaling, isBlast);
	// Finish the blast radius calculations.
	if(isBlast && weapon.IsDamageScaled())
	{
		// Rather than exactly compute the distance between the explosion and
		// the closest point on the ship, estimate it using the mask's Radius.
		double d = max(0., (position - ship.Position()).Length() - ship.GetMask().Radius());
		double finalR = d * d * rSquared;
		damage.scaling *= k / ((1. + finalR * finalR) * (1. + finalR * finalR));
	}
	// If damage falloff scaling was skipped before, compute it now.
	if(skipFalloff && weapon.HasDamageDropoff())
		damage.scaling *= weapon.DamageDropoff(distanceTraveled);

	double shieldFraction = 0.;

	// Lambda for returning the damage scale that a damage type should
	// use given the default percentage that is blocked by shields and
	// the value of its protection attribute.
	auto ScaleType = [&](double blocked, double protection)
	{
		return damage.scaling * (1. - blocked * shieldFraction) / (1. + protection);
	};

	// Determine the shieldFraction.
	if(shields > 0.)
	{
		double piercing = max(0., min(1., weapon.Piercing() / (1. + attributes.Get("piercing protection")) - attributes.Get("piercing resistance")));
		shieldFraction = (1. - piercing) / (1. + disrupted * .01);

		damage.shieldDamage = (weapon.ShieldDamage()
			+ weapon.RelativeShieldDamage() * attributes.Get("shields"))
			* ScaleType(0., attributes.Get("shield protection"));
		if(damage.shieldDamage > shields)
			shieldFraction = min(shieldFraction, shields / damage.shieldDamage);
	}

	// Instantaneous damage types.
	// Energy, heat, and fuel damage are blocked 50% by shields.
	// Hull damage is blocked 100%.
	// Shield damage is blocked 0%.
	damage.shieldDamage *= shieldFraction;
	damage.hullDamage = (weapon.HullDamage()
		+ weapon.RelativeHullDamage() * attributes.Get("hull"))
		* ScaleType(1., attributes.Get("hull protection"));
	damage.energyDamage = (weapon.EnergyDamage()
		+ weapon.RelativeEnergyDamage() * attributes.Get("energy capacity"))
		* ScaleType(.5, attributes.Get("energy protection"));
	damage.heatDamage = (weapon.HeatDamage()
		+ weapon.RelativeHeatDamage() * ship.MaximumHeat())
		* ScaleType(.5, attributes.Get("heat protection"));
	damage.fuelDamage = (weapon.FuelDamage()
		+ weapon.RelativeFuelDamage() * attributes.Get("fuel capacity"))
		* ScaleType(.5, attributes.Get("fuel protection"));

	// DoT damage types with an instantaneous analog.
	// Ion and burn damage are blocked 50% by shields.
	// Corrosion and leak damage are blocked 100%.
	// Discharge damage is blocked 0%.
	damage.dischargeDamage = weapon.DischargeDamage() * ScaleType(0., attributes.Get("discharge protection"));
	damage.corrosionDamage = weapon.CorrosionDamage() * ScaleType(1., attributes.Get("corrosion protection"));
	damage.ionDamage = weapon.IonDamage() * ScaleType(.5, attributes.Get("ion protection"));
	damage.burnDamage = weapon.BurnDamage() * ScaleType(.5, attributes.Get("burn protection"));
	damage.leakDamage = weapon.LeakDamage() * ScaleType(1., attributes.Get("leak protection"));

	// Unique special damage types.
	// Disruption and slowing are blocked 50% by shields.
	damage.disruptionDamage = weapon.DisruptionDamage() * ScaleType(.5, attributes.Get("disruption protection"));
	damage.slowingDamage = weapon.SlowingDamage() * ScaleType(.5, attributes.Get("slowing protection"));

	// Hit force is blocked 0% by shields.
	double hitForce = weapon.HitForce() * ScaleType(0., attributes.Get("force protection"));
	if(hitForce)
	{
		Point d = ship.Position() - position;
		double distance = d.Length();
		if(distance)
			damage.forcePoint = (hitForce / distance) * d;
	}

	return damage;
}
