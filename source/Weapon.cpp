/* Weapon.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Weapon.h"

#include "Audio.h"
#include "DataNode.h"
#include "Effect.h"
#include "GameData.h"
#include "Outfit.h"
#include "SpriteSet.h"

#include <algorithm>

using namespace std;



// Load from a "weapon" node, either in an outfit or in a ship (explosion).
void Weapon::LoadWeapon(const DataNode &node)
{
	isWeapon = true;
	bool isClustered = false;
	calculatedDamage = false;
	doesDamage = false;
	bool safeRangeOverriden = false;
	bool disabledDamageSet = false;
	bool minableDamageSet = false;
	bool relativeDisabledDamageSet = false;
	bool relativeMinableDamageSet = false;

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "stream")
			isStreamed = true;
		else if(key == "cluster")
			isClustered = true;
		else if(key == "safe")
			isSafe = true;
		else if(key == "phasing")
			isPhasing = true;
		else if(key == "no damage scaling")
			isDamageScaled = false;
		else if(key == "parallel")
			isParallel = true;
		else if(key == "gravitational")
			isGravitational = true;
		else if(child.Size() < 2)
			child.PrintTrace("Skipping weapon attribute with no value specified:");
		else if(key == "sprite")
			sprite.LoadSprite(child);
		else if(key == "hardpoint sprite")
			hardpointSprite.LoadSprite(child);
		else if(key == "sound")
			sound = Audio::Get(child.Token(1));
		else if(key == "ammo")
		{
			int usage = (child.Size() >= 3) ? child.Value(2) : 1;
			ammo = make_pair(GameData::Outfits().Get(child.Token(1)), max(0, usage));
		}
		else if(key == "icon")
			icon = SpriteSet::Get(child.Token(1));
		else if(key == "fire effect")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			fireEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(key == "live effect")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			liveEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(key == "hit effect")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			hitEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(key == "target effect")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			targetEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(key == "die effect")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			dieEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(key == "submunition")
		{
			submunitions.emplace_back(
				GameData::Outfits().Get(child.Token(1)),
				(child.Size() >= 3) ? child.Value(2) : 1);
			for(const DataNode &grand : child)
			{
				if((grand.Size() >= 2) && (grand.Token(0) == "facing"))
					submunitions.back().facing = Angle(grand.Value(1));
				else if((grand.Size() >= 3) && (grand.Token(0) == "offset"))
					submunitions.back().offset = Point(grand.Value(1), grand.Value(2));
				else
					child.PrintTrace("Skipping unknown or incomplete submunition attribute:");
			}
		}
		else if(key == "inaccuracy")
		{
			inaccuracy = child.Value(1);
			for(const DataNode &grand : child)
			{
				for(int j = 0; j < grand.Size(); ++j)
				{
					const string &token = grand.Token(j);

					if(token == "inverted")
						inaccuracyDistribution.second = true;
					else if(token == "triangular")
						inaccuracyDistribution.first = Distribution::Type::Triangular;
					else if(token == "uniform")
						inaccuracyDistribution.first = Distribution::Type::Uniform;
					else if(token == "narrow")
						inaccuracyDistribution.first = Distribution::Type::Narrow;
					else if(token == "medium")
						inaccuracyDistribution.first = Distribution::Type::Medium;
					else if(token == "wide")
						inaccuracyDistribution.first = Distribution::Type::Wide;
					else
						grand.PrintTrace("Skipping unknown distribution attribute:");
				}
			}
		}
		else
		{
			double value = child.Value(1);
			if(key == "lifetime")
				lifetime = max(0., value);
			else if(key == "random lifetime")
				randomLifetime = max(0., value);
			else if(key == "reload")
				reload = max(1., value);
			else if(key == "burst reload")
				burstReload = max(1., value);
			else if(key == "burst count")
				burstCount = max(1., value);
			else if(key == "homing")
				homing = value;
			else if(key == "missile strength")
				missileStrength = max(0., value);
			else if(key == "anti-missile")
				antiMissile = max(0., value);
			else if(key == "velocity")
				velocity = value;
			else if(key == "random velocity")
				randomVelocity = value;
			else if(key == "acceleration")
				acceleration = value;
			else if(key == "drag")
				drag = value;
			else if(key == "hardpoint offset")
			{
				// A single value specifies the y-offset, while two values
				// specifies an x & y offset, e.g. for an asymmetric hardpoint.
				// The point is specified in traditional XY orientation, but must
				// be inverted along the y-dimension for internal use.
				if(child.Size() == 2)
					hardpointOffset = Point(0., -value);
				else if(child.Size() == 3)
					hardpointOffset = Point(value, -child.Value(2));
				else
					child.PrintTrace("Unsupported \"" + key + "\" specification:");
			}
			else if(key == "turn")
				turn = value;
			else if(key == "turret turn")
				turretTurn = value;
			else if(key == "tracking")
				tracking = max(0., min(1., value));
			else if(key == "optical tracking")
				opticalTracking = max(0., min(1., value));
			else if(key == "infrared tracking")
				infraredTracking = max(0., min(1., value));
			else if(key == "radar tracking")
				radarTracking = max(0., min(1., value));
			else if(key == "firing energy")
				firingEnergy = value;
			else if(key == "firing force")
				firingForce = value;
			else if(key == "firing fuel")
				firingFuel = value;
			else if(key == "firing heat")
				firingHeat = value;
			else if(key == "firing hull")
				firingHull = value;
			else if(key == "firing shields")
				firingShields = value;
			else if(key == "firing ion")
				firingIon = value;
			else if(key == "firing scramble")
				firingScramble = value;
			else if(key == "firing slowing")
				firingSlowing = value;
			else if(key == "firing disruption")
				firingDisruption = value;
			else if(key == "firing discharge")
				firingDischarge = value;
			else if(key == "firing corrosion")
				firingCorrosion = value;
			else if(key == "firing leak")
				firingLeak = value;
			else if(key == "firing burn")
				firingBurn = value;
			else if(key == "relative firing energy")
				relativeFiringEnergy = value;
			else if(key == "relative firing heat")
				relativeFiringHeat = value;
			else if(key == "relative firing fuel")
				relativeFiringFuel = value;
			else if(key == "relative firing hull")
				relativeFiringHull = value;
			else if(key == "relative firing shields")
				relativeFiringShields = value;
			else if(key == "split range")
				splitRange = max(0., value);
			else if(key == "trigger radius")
				triggerRadius = max(0., value);
			else if(key == "blast radius")
				blastRadius = max(0., value);
			else if(key == "safe range override")
			{
				safeRange = max(0., value);
				safeRangeOverriden = true;
			}
			else if(key == "shield damage")
				damage[SHIELD_DAMAGE] = value;
			else if(key == "hull damage")
				damage[HULL_DAMAGE] = value;
			else if(key == "disabled damage")
			{
				damage[DISABLED_DAMAGE] = value;
				disabledDamageSet = true;
			}
			else if(key == "minable damage")
			{
				damage[MINABLE_DAMAGE] = value;
				minableDamageSet = true;
			}
			else if(key == "fuel damage")
				damage[FUEL_DAMAGE] = value;
			else if(key == "heat damage")
				damage[HEAT_DAMAGE] = value;
			else if(key == "energy damage")
				damage[ENERGY_DAMAGE] = value;
			else if(key == "ion damage")
				damage[ION_DAMAGE] = value;
			else if(key == "scrambling damage")
				damage[WEAPON_JAMMING_DAMAGE] = value;
			else if(key == "disruption damage")
				damage[DISRUPTION_DAMAGE] = value;
			else if(key == "slowing damage")
				damage[SLOWING_DAMAGE] = value;
			else if(key == "discharge damage")
				damage[DISCHARGE_DAMAGE] = value;
			else if(key == "corrosion damage")
				damage[CORROSION_DAMAGE] = value;
			else if(key == "leak damage")
				damage[LEAK_DAMAGE] = value;
			else if(key == "burn damage")
				damage[BURN_DAMAGE] = value;
			else if(key == "relative shield damage")
				damage[RELATIVE_SHIELD_DAMAGE] = value;
			else if(key == "relative hull damage")
				damage[RELATIVE_HULL_DAMAGE] = value;
			else if(key == "relative disabled damage")
			{
				damage[RELATIVE_DISABLED_DAMAGE] = value;
				relativeDisabledDamageSet = true;
			}
			else if(key == "relative minable damage")
			{
				damage[RELATIVE_MINABLE_DAMAGE] = value;
				relativeMinableDamageSet = true;
			}
			else if(key == "relative fuel damage")
				damage[RELATIVE_FUEL_DAMAGE] = value;
			else if(key == "relative heat damage")
				damage[RELATIVE_HEAT_DAMAGE] = value;
			else if(key == "relative energy damage")
				damage[RELATIVE_ENERGY_DAMAGE] = value;
			else if(key == "hit force")
				damage[HIT_FORCE] = value;
			else if(key == "piercing")
				piercing = max(0., value);
			else if(key == "range override")
				rangeOverride = max(0., value);
			else if(key == "velocity override")
				velocityOverride = max(0., value);
			else if(key == "damage dropoff")
			{
				hasDamageDropoff = true;
				double maxDropoff = (child.Size() >= 3) ? child.Value(2) : 0.;
				damageDropoffRange = make_pair(max(0., value), maxDropoff);
			}
			else if(key == "dropoff modifier")
				damageDropoffModifier = max(0., value);
			else
				child.PrintTrace("Unrecognized weapon attribute: \"" + key + "\":");
		}
	}
	// Disabled damage defaults to hull damage instead of 0.
	if(!disabledDamageSet)
		damage[DISABLED_DAMAGE] = damage[HULL_DAMAGE];
	if(!relativeDisabledDamageSet)
		damage[RELATIVE_DISABLED_DAMAGE] = damage[RELATIVE_HULL_DAMAGE];
	// Minable damage defaults to hull damage instead of 0.
	if(!minableDamageSet)
		damage[MINABLE_DAMAGE] = damage[HULL_DAMAGE];
	if(!relativeMinableDamageSet)
		damage[RELATIVE_MINABLE_DAMAGE] = damage[RELATIVE_HULL_DAMAGE];

	// Sanity checks:
	if(burstReload > reload)
		burstReload = reload;
	if(damageDropoffRange.first > damageDropoffRange.second)
		damageDropoffRange.second = Range();

	// Weapons of the same type will alternate firing (streaming) rather than
	// firing all at once (clustering) if the weapon is not an anti-missile and
	// is not vulnerable to anti-missile, or has the "stream" attribute.
	isStreamed |= !(MissileStrength() || AntiMissile());
	isStreamed &= !isClustered;

	// Support legacy missiles with no tracking type defined:
	if(homing && !tracking && !opticalTracking && !infraredTracking && !radarTracking)
	{
		tracking = 1.;
		node.PrintTrace("Warning: Deprecated use of \"homing\" without use of \"[optical|infrared|radar] tracking.\"");
	}

	// Convert the "live effect" counts from occurrences per projectile lifetime
	// into chance of occurring per frame.
	if(lifetime <= 0)
		liveEffects.clear();
	for(auto it = liveEffects.begin(); it != liveEffects.end(); )
	{
		if(!it->second)
			it = liveEffects.erase(it);
		else
		{
			it->second = max(1, lifetime / it->second);
			++it;
		}
	}

	// Only when the weapon is not safe and has a blast radius is safeRange needed,
	// except if it is already overridden.
	if(!isSafe && blastRadius > 0 && !safeRangeOverriden)
		safeRange = (blastRadius + triggerRadius);
}



bool Weapon::IsWeapon() const
{
	return isWeapon;
}



// Get assets used by this weapon.
const Body &Weapon::WeaponSprite() const
{
	return sprite;
}



const Body &Weapon::HardpointSprite() const
{
	return hardpointSprite;
}



const Sound *Weapon::WeaponSound() const
{
	return sound;
}



const Outfit *Weapon::Ammo() const
{
	return ammo.first;
}



int Weapon::AmmoUsage() const
{
	return ammo.second;
}



bool Weapon::IsParallel() const
{
	return isParallel;
}



const Sprite *Weapon::Icon() const
{
	return icon;
}



// Effects to be created at the start or end of the weapon's lifetime.
const map<const Effect *, int> &Weapon::FireEffects() const
{
	return fireEffects;
}



const map<const Effect *, int> &Weapon::LiveEffects() const
{
	return liveEffects;
}



const map<const Effect *, int> &Weapon::HitEffects() const
{
	return hitEffects;
}



const map<const Effect *, int> &Weapon::TargetEffects() const
{
	return targetEffects;
}



const map<const Effect *, int> &Weapon::DieEffects() const
{
	return dieEffects;
}



const vector<Weapon::Submunition> &Weapon::Submunitions() const
{
	return submunitions;
}



double Weapon::TotalLifetime() const
{
	if(rangeOverride)
		return rangeOverride / WeightedVelocity();
	if(totalLifetime < 0.)
	{
		totalLifetime = 0.;
		for(const auto &it : submunitions)
			totalLifetime = max(totalLifetime, it.weapon->TotalLifetime());
		totalLifetime += lifetime;
	}
	return totalLifetime;
}



double Weapon::Range() const
{
	return (rangeOverride > 0) ? rangeOverride : WeightedVelocity() * TotalLifetime();
}



// Calculate the fraction of full damage that this weapon deals given the
// distance that the projectile traveled if it has a damage dropoff range.
double Weapon::DamageDropoff(double distance) const
{
	double minDropoff = damageDropoffRange.first;
	double maxDropoff = damageDropoffRange.second;

	if(distance <= minDropoff)
		return 1.;
	if(distance >= maxDropoff)
		return damageDropoffModifier;
	// Damage modification is linear between the min and max dropoff points.
	double slope = (1 - damageDropoffModifier) / (minDropoff - maxDropoff);
	return slope * (distance - minDropoff) + 1;
}



// Legacy support: allow turret outfits with no turn rate to specify a
// default turnrate.
void Weapon::SetTurretTurn(double rate)
{
	turretTurn = rate;
}



double Weapon::TotalDamage(int index) const
{
	if(!calculatedDamage)
	{
		calculatedDamage = true;
		for(int i = 0; i < DAMAGE_TYPES; ++i)
		{
			for(const auto &it : submunitions)
				damage[i] += it.weapon->TotalDamage(i) * it.count;
			doesDamage |= (damage[i] > 0.);
		}
	}
	return damage[index];
}



pair<Distribution::Type, bool> Weapon::InaccuracyDistribution() const
{
	return inaccuracyDistribution;
}



double Weapon::Inaccuracy() const
{
	return inaccuracy;
}
