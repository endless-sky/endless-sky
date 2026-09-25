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
#include "Entity.h"
#include "image/Mask.h"
#include "Weapon.h"

using namespace std;



DamageProfile::DamageProfile(const Entity &entity, const Projectile::ImpactInfo &info, bool ignoreBlast)
	: entity(&entity), weapon(&info.weapon), position(info.position),
	isBlast(!ignoreBlast && weapon->BlastRadius() > 0.)
{
	// The damage dropoff of projectiles is influenced by the distance it traveled.
	if(weapon->HasDamageDropoff())
		scaling *= weapon->DamageDropoff(info.distanceTraveled);
	CalculateScaling();
}



DamageProfile::DamageProfile(const Entity &entity, const Weather::ImpactInfo &info, bool ignoreBlast)
	: entity(&entity), weapon(&info.weapon), position(info.position),
	isBlast(!ignoreBlast && weapon->BlastRadius() > 0.), scaling(info.scale), isHazard(true)
{
	CalculateScaling();
}



DamageProfile::DamageProfile(const Entity &entity, const Weapon &weapon)
	: entity(&entity), weapon(&weapon), position(Point()), isBlast(false)
{
}



const Weapon &DamageProfile::GetWeapon() const
{
	return *weapon;
}



const Entity &DamageProfile::GetEntity() const
{
	return *entity;
}



double DamageProfile::Scaling() const
{
	return scaling;
}



DamageDealt DamageProfile::CalculateDamage() const
{
	const Weapon &weapon = GetWeapon();
	const Entity &entity = GetEntity();

	double shieldFraction = 0.;
	DamageDealt damage(weapon, scaling);

	// Lambda for returning the damage scale that a damage type should
	// use given the default percentage that is blocked by shields and hull,
	// and the value of its protection attribute.
	auto ScaleType = [&](double shieldBlocked, double hullBlocked, double protection)
	{
		double blocked = (1. - shieldBlocked) * (shieldFraction) + (1. - hullBlocked) * (1. - shieldFraction);
		return scaling * blocked / protection;
	};

	// Determine the shieldFraction, which dictates how much damage
	// bleeds through the shields that would normally be blocked.
	double shields = entity.ShieldLevel();
	if(shields > 0.)
	{
		double piercing = max(0., min(1., (weapon.Piercing() / entity.PiercingProtection()) - entity.PiercingResistance()));
		double highPermeability = entity.HighShieldPermeability();
		double lowPermeability = entity.LowShieldPermeability();
		double permeability = entity.Cloaking() * entity.CloakedShieldPermeability();
		if(highPermeability || lowPermeability)
		{
			// Determine what portion of its maximum shields the entity is currently at.
			// Only do this if there is nonzero permeability involved, otherwise don't.
			double shieldPortion = shields / entity.MaxShields();
			permeability += max((highPermeability * shieldPortion) + (lowPermeability * (1. - shieldPortion)), 0.);
		}
		shieldFraction = (1. - min(piercing + permeability, 1.)) / (1. + entity.DisruptionLevel() * .01);

		damage.levels.shields = (weapon.ShieldDamage() + weapon.RelativeShieldDamage() * entity.MaxShields())
			* ScaleType(0., 0., entity.DamageProtection().shields
			+ (entity.IsCloaked() ? entity.CloakedShieldProtection() : 0.));
		if(damage.levels.shields > shields)
			shieldFraction = min(shieldFraction, shields / damage.levels.shields);
	}

	// Instantaneous damage types.
	// Energy, heat, and fuel damage are blocked 50% by shields.
	// Hull damage is blocked 100%.
	// Shield damage is blocked 0%.
	damage.levels.shields *= shieldFraction;
	double totalHullProtection = (ScaleType(1., 0., entity.DamageProtection().hull +
		(entity.IsCloaked() ? entity.CloakedHullProtection() : 0.)));
	damage.levels.hull = (weapon.HullDamage() + weapon.RelativeHullDamage() * entity.MaxHull())
		* totalHullProtection;
	double hull = entity.HullLevelUntilDisabled();
	if(damage.levels.hull > hull)
	{
		double hullFraction = hull / damage.levels.hull;
		damage.levels.hull *= hullFraction;
		damage.levels.hull += (weapon.DisabledDamage() + weapon.RelativeDisabledDamage() * entity.MaxHull())
			* totalHullProtection * (1. - hullFraction);
	}
	damage.levels.energy = (weapon.EnergyDamage() + weapon.RelativeEnergyDamage() * entity.MaxEnergy())
		* ScaleType(.5, 0., entity.DamageProtection().energy);
	damage.levels.heat = (weapon.HeatDamage() + weapon.RelativeHeatDamage() * entity.MaxHeat())
		* ScaleType(.5, 0., entity.DamageProtection().heat);
	damage.levels.fuel = (weapon.FuelDamage() + weapon.RelativeFuelDamage() * entity.MaxFuel())
		* ScaleType(.5, 0., entity.DamageProtection().fuel);

	// DoT damage types with an instantaneous analog.
	// Ion and burn damage are blocked 50% by shields.
	// Corrosion and leak damage are blocked 100%.
	// Discharge damage is blocked 50% by the absence of shields.
	damage.levels.discharge = weapon.DischargeDamage() * ScaleType(0., .5, entity.DamageProtection().discharge);
	damage.levels.corrosion = weapon.CorrosionDamage() * ScaleType(1., 0., entity.DamageProtection().corrosion);
	damage.levels.ionization = weapon.IonDamage() * ScaleType(.5, 0., entity.DamageProtection().ionization);
	damage.levels.burning = weapon.BurnDamage() * ScaleType(.5, 0., entity.DamageProtection().burning);
	damage.levels.leakage = weapon.LeakDamage() * ScaleType(1., 0., entity.DamageProtection().leakage);

	// Unique special damage types.
	// Slowing and scrambling are blocked 50% by shields.
	// Disruption is blocked 50% by the absence of shields.
	damage.levels.slowness = weapon.SlowingDamage() * ScaleType(.5, 0., entity.DamageProtection().slowness);
	damage.levels.scrambling = weapon.ScramblingDamage() * ScaleType(.5, 0., entity.DamageProtection().scrambling);
	damage.levels.disruption = weapon.DisruptionDamage() * ScaleType(0., .5, entity.DamageProtection().disruption);

	// Hit force is unaffected by shields.
	double hitForce = weapon.HitForce() * ScaleType(0., 0., entity.ForceProtection());
	if(hitForce)
	{
		Point d = entity.Position() - position;
		double distance = d.Length();
		if(distance)
			damage.forcePoint = (hitForce / distance) * d;
	}

	// Prospecting is unaffected by anything aside from the base damage scaling right now.
	damage.prospecting = weapon.Prospecting() * scaling;

	return damage;
}



void DamageProfile::CalculateScaling()
{
	// Always use the closest possible point between the impact position and the entity
	// by using the radius of the mask, as opposed to considering how the entity was
	// rotated relative to the impact position.
	double distance = max(0., position.Distance(entity->Position()) - entity->GetMask().Radius());

	if(isBlast && weapon->IsDamageScaled())
	{
		// Scale blast damage based on the distance from the blast
		// origin and if the projectile uses a trigger radius. The
		// point of contact must be measured on the sprite outline.
		// scale = (1 + (tr / (2 * br))^2) / (1 + r^4)^2
		double blastRadius = max(1., weapon->BlastRadius());
		double radiusRatio = weapon->TriggerRadius() / blastRadius;
		double k = !radiusRatio ? 1. : (1. + .25 * radiusRatio * radiusRatio);
		double rSquared = 1. / (blastRadius * blastRadius);

		// Rather than exactly compute the distance between the explosion and
		// the closest point on the entity, estimate it using the mask's Radius.
		double finalR = distance * distance * rSquared;
		scaling *= k / ((1. + finalR * finalR) * (1. + finalR * finalR));
	}
	// The damage dropoff of hazards is influenced by the distance to the target.
	if(isHazard && weapon->HasDamageDropoff())
		scaling *= weapon->DamageDropoff(distance);
}
