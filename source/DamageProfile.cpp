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

#include "Outfit.h"
#include "Ship.h"
#include "Weapon.h"

using namespace std;

DamageProfile::DamageProfile(const Ship &ship, const Weapon &weapon, double scaling, double shields, double disrupted)
	: attributes(ship.Attributes()), scaling(scaling)
{
	if(shields <= 0.)
		shieldFraction = 0.;
	else
	{
		double piercing = max(0., min(1., weapon.Piercing() / (1. + attributes.Get("piercing protection")) - attributes.Get("piercing resistance")));
		shieldFraction = (1. - piercing) / (1. + disrupted * .01);
		
		shieldDamage = (weapon.ShieldDamage()
			+ weapon.RelativeShieldDamage() * attributes.Get("shields"))
			* Scale(0., "shield");
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
		* Scale(1., "hull");
	energyDamage = (weapon.EnergyDamage()
		+ weapon.RelativeEnergyDamage() * attributes.Get("energy capacity"))
		* Scale(.5, "energy");
	heatDamage = (weapon.HeatDamage()
		+ weapon.RelativeHeatDamage() * ship.MaximumHeat())
		* Scale(.5, "heat");
	fuelDamage = (weapon.FuelDamage()
		+ weapon.RelativeFuelDamage() * attributes.Get("fuel capacity"))
		* Scale(.5, "fuel");

	// DoT damage types with an instantaneous analog.
	// Ion and burn damage are blocked 50% by shields.
	// Corrosion and leak damage are blocked 100%.
	// Discharge damage is blocked 0%.
	dischargeDamage = weapon.DischargeDamage() * Scale(0., "discharge");
	corrosionDamage = weapon.CorrosionDamage() * Scale(1., "corrosion");
	ionDamage = weapon.IonDamage() * Scale(.5, "ion");
	burnDamage = weapon.BurnDamage() * Scale(.5, "burn");
	leakDamage = weapon.LeakDamage() * Scale(1., "leak");

	// Unique special damage types.
	// Disruption and slowing are blocked 50% by shields.
	// Hit force is blocked 0%.
	disruptionDamage = weapon.DisruptionDamage() * Scale(.5, "disruption");
	slowingDamage = weapon.SlowingDamage() * Scale(.5, "slowing");
	hitForce = weapon.HitForce() * Scale(0., "force");
}



// Return the damage scale that a damage type should use given the
// default percentage that is blocked by shields and the name of
// its protection attribute.
double DamageProfile::Scale(double blocked, const string &attr) const
{
	return scaling * (1. - blocked * shieldFraction) / (1. + attributes.Get(attr + " protection"));
}
