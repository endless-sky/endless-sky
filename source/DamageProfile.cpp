/* DamageProfile.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "DamageProfile.h"

#include "DamageDealt.h"
#include "image/Mask.h"
#include "Minable.h"
#include "MinableDamageDealt.h"
#include "Outfit.h"
#include "Ship.h"
#include "ship/ShipAttributeHandler.h"
#include "Weapon.h"

using namespace std;



DamageProfile::DamageProfile(Projectile::ImpactInfo info)
	: weapon(info.weapon), position(std::move(info.position)), isBlast(weapon.BlastRadius() > 0.)
{
	CalculateBlast();
	// For weapon projectiles, the distance traveled for the projectile
	// is the same regardless of the ship being impacted, so calculate
	// its effect on the damage scale here.
	if(weapon.HasDamageDropoff())
		inputScaling *= weapon.DamageDropoff(info.distanceTraveled);
}



DamageProfile::DamageProfile(Weather::ImpactInfo info)
	: weapon(info.weapon), position(std::move(info.position)), isBlast(weapon.BlastRadius() > 0.), inputScaling(info.scale)
{
	CalculateBlast();
	isHazard = true;
}



// Calculate the damage dealt to the given ship.
DamageDealt DamageProfile::CalculateDamage(const Ship &ship, bool ignoreBlast) const
{
	bool blast = (isBlast && !ignoreBlast);
	DamageDealt damage(weapon, Scale(inputScaling, ship, blast));
	PopulateDamage(damage, ship);

	return damage;
}



MinableDamageDealt DamageProfile::CalculateDamage(const Minable &minable) const
{
	double scale = Scale(inputScaling, minable, isBlast);
	return {scale * (weapon.MinableDamage() + weapon.RelativeMinableDamage() * minable.MaxHull()),
		scale * weapon.Prospecting()};
}



// Calculate the value of certain variables necessary for determining
// the impact of an explosion that are shared across all ships that
// this hazard could impact.
void DamageProfile::CalculateBlast()
{
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



// Determine the damage scale for the given body.
double DamageProfile::Scale(double scale, const Body &body, bool blast) const
{
	// Now that we have a specific ship, we can finish the blast damage
	// calculations.
	if(blast && weapon.IsDamageScaled())
	{
		// Rather than exactly compute the distance between the explosion and
		// the closest point on the ship, estimate it using the mask's Radius.
		double distance = max(0., position.Distance(body.Position()) - body.GetMask().Radius());
		double finalR = distance * distance * rSquared;
		scale *= k / ((1. + finalR * finalR) * (1. + finalR * finalR));
	}
	// Hazards must wait to evaluate any damage dropoff until now as the ship
	// position for each ship influences the distance used for the damage dropoff.
	if(isHazard && weapon.HasDamageDropoff())
	{
		double distance = max(0., position.Distance(body.Position()) - body.GetMask().Radius());
		scale *= weapon.DamageDropoff(distance);
	}

	return scale;
}



// Populate the given DamageDealt object with values.
void DamageProfile::PopulateDamage(DamageDealt &damage, const Ship &ship) const
{
	const ShipAttributeHandler &attrHandler = ship.AttributeHandler();
	double shieldFraction = 0.;

	// Lambda for returning the damage scale that a damage type should
	// use given the default percentage that is blocked by shields and hull,
	// and the value of its protection attribute.
	auto ScaleType = [&](double shieldBlocked, double hullBlocked, double protection)
	{
		double blocked = (1. - shieldBlocked) * (shieldFraction) + (1. - hullBlocked) * (1. - shieldFraction);
		return damage.scaling * blocked / protection;
	};

	// Determine the shieldFraction, which dictates how much damage
	// bleeds through the shields that would normally be blocked.
	double shields = ship.ShieldLevel();
	if(shields > 0.)
	{
		double piercing = max(0., min(1., (weapon.Piercing() / attrHandler.PiercingProtection())
			- attrHandler.PiercingResistance()));
		double highPermeability = attrHandler.HighShieldPermeability();
		double lowPermeability = attrHandler.LowShieldPermeability();
		double permeability = ship.Cloaking() * attrHandler.CloakedShieldPermeability();
		if(highPermeability || lowPermeability)
		{
			// Determine what portion of its maximum shields the ship is currently at.
			// Only do this if there is nonzero permeability involved, otherwise don't.
			double shieldPortion = shields / ship.MaxShields();
			permeability += max((highPermeability * shieldPortion) +
				(lowPermeability * (1. - shieldPortion)), 0.);
		}
		shieldFraction = (1. - min(piercing + permeability, 1.)) / (1. + ship.DisruptionLevel() * .01);

		damage.levels.shields = (weapon.ShieldDamage()
			+ weapon.RelativeShieldDamage() * ship.MaxShields())
			* ScaleType(0., 0., attrHandler.DamageProtection().shields
				+ (ship.IsCloaked() ? attrHandler.CloakedShieldProtection() : 0.));
		if(damage.levels.shields > shields)
			shieldFraction = min(shieldFraction, shields / damage.levels.shields);
	}

	// Instantaneous damage types.
	// Energy, heat, and fuel damage are blocked 50% by shields.
	// Hull damage is blocked 100%.
	// Shield damage is blocked 0%.
	damage.levels.shields *= shieldFraction;
	double totalHullProtection = (ScaleType(1., 0., attrHandler.DamageProtection().hull +
		(ship.IsCloaked() ? attrHandler.CloakedHullProtection() : 0.)));
	damage.levels.hull = (weapon.HullDamage()
		+ weapon.RelativeHullDamage() * ship.MaxHull())
		* totalHullProtection;
	double hull = ship.HullUntilDisabled();
	if(damage.levels.hull > hull)
	{
		double hullFraction = hull / damage.levels.hull;
		damage.levels.hull *= hullFraction;
		damage.levels.hull += (weapon.DisabledDamage()
			+ weapon.RelativeDisabledDamage() * ship.MaxHull())
			* totalHullProtection
			* (1. - hullFraction);
	}
	damage.levels.energy = (weapon.EnergyDamage()
		+ weapon.RelativeEnergyDamage() * ship.MaxEnergy())
		* ScaleType(.5, 0., attrHandler.DamageProtection().energy);
	damage.levels.heat = (weapon.HeatDamage()
		+ weapon.RelativeHeatDamage() * ship.MaxHeat())
		* ScaleType(.5, 0., attrHandler.DamageProtection().heat);
	damage.levels.fuel = (weapon.FuelDamage()
		+ weapon.RelativeFuelDamage() * ship.MaxFuel())
		* ScaleType(.5, 0., attrHandler.DamageProtection().fuel);

	// DoT damage types with an instantaneous analog.
	// Ion and burn damage are blocked 50% by shields.
	// Corrosion and leak damage are blocked 100%.
	// Discharge damage is blocked 50% by the absence of shields.
	damage.levels.discharge = weapon.DischargeDamage() * ScaleType(0., .5, attrHandler.DamageProtection().discharge);
	damage.levels.corrosion = weapon.CorrosionDamage() * ScaleType(1., 0., attrHandler.DamageProtection().corrosion);
	damage.levels.ionization = weapon.IonDamage() * ScaleType(.5, 0., attrHandler.DamageProtection().ionization);
	damage.levels.burning = weapon.BurnDamage() * ScaleType(.5, 0., attrHandler.DamageProtection().burning);
	damage.levels.leakage = weapon.LeakDamage() * ScaleType(1., 0., attrHandler.DamageProtection().leakage);

	// Unique special damage types.
	// Slowing and scrambling are blocked 50% by shields.
	// Disruption is blocked 50% by the absence of shields.
	damage.levels.slowness = weapon.SlowingDamage() * ScaleType(.5, 0., attrHandler.DamageProtection().slowness);
	damage.levels.scrambling = weapon.ScramblingDamage() * ScaleType(.5, 0., attrHandler.DamageProtection().scrambling);
	damage.levels.disruption = weapon.DisruptionDamage() * ScaleType(0., .5, attrHandler.DamageProtection().disruption);

	// Hit force is unaffected by shields.
	double hitForce = weapon.HitForce() * ScaleType(0., 0., attrHandler.ForceProtection());
	if(hitForce)
	{
		Point d = ship.Position() - position;
		double distance = d.Length();
		if(distance)
			damage.forcePoint = (hitForce / distance) * d;
	}
}
