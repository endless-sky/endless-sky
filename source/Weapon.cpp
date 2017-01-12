/* Weapon.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Weapon.h"

#include "Audio.h"
#include "DataNode.h"
#include "Effect.h"
#include "GameData.h"
#include "Outfit.h"
#include "SpriteSet.h"

using namespace std;



// Load from a "weapon" node, either in an outfit or in a ship (explosion).
void Weapon::LoadWeapon(const DataNode &node)
{
	isWeapon = true;
	bool isClustered = false;
	
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "stream")
			isStreamed = true;
		else if(key == "cluster")
			isClustered = true;
		else if(child.Size() < 2)
			child.PrintTrace("Skipping weapon attribute with no value specified:");
		else if(key == "sprite")
			sprite.LoadSprite(child);
		else if(key == "hardpoint sprite")
			hardpointSprite.LoadSprite(child);
		else if(key == "sound")
			sound = Audio::Get(child.Token(1));
		else if(key == "ammo")
			ammo = GameData::Outfits().Get(child.Token(1));
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
		else if(key == "die effect")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			dieEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(key == "submunition")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			submunitions[GameData::Outfits().Get(child.Token(1))] += count;
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
				hardpointOffset = value;
			else if(key == "turn")
				turn = value;
			else if(key == "inaccuracy")
				inaccuracy = value;
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
			else if(key == "split range")
				splitRange = value;
			else if(key == "trigger radius")
				triggerRadius = value;
			else if(key == "blast radius")
				blastRadius = value;
			else if(key == "shield damage")
				damage[SHIELD_DAMAGE] = value;
			else if(key == "hull damage")
				damage[HULL_DAMAGE] = value;
			else if(key == "heat damage")
				damage[HEAT_DAMAGE] = value;
			else if(key == "ion damage")
				damage[ION_DAMAGE] = value;
			else if(child.Token(0) == "fuel damage")
				damage[FUEL_DAMAGE] = child.Value(1);
			else if(key == "disruption damage")
				damage[DISRUPTION_DAMAGE] = value;
			else if(key == "slowing damage")
				damage[SLOWING_DAMAGE] = value;
			else if(key == "hit force")
				hitForce = value;
			else if(key == "piercing")
				piercing = max(0., min(1., value));
			else
				child.PrintTrace("Unrecognized weapon attribute: \"" + key + "\":");
		}
	}
	// Sanity check:
	if(burstReload > reload)
		burstReload = reload;
	
	// Weapons of the same type will alternate firing (streaming) rather than
	// firing all at once (clustering) if the weapon is not an anti-missile and
	// is not vulnerable to anti-missile, or has the "stream" attribute.
	isStreamed |= !(MissileStrength() || AntiMissile());
	isStreamed &= !isClustered;
	
	// Support legacy missiles with no tracking type defined:
	if(homing && !tracking && !opticalTracking && !infraredTracking && !radarTracking)
		tracking = 1.;
	
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
	return ammo;
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



const map<const Effect *, int> &Weapon::DieEffects() const
{
	return dieEffects;
}



const map<const Outfit *, int> &Weapon::Submunitions() const
{
	return submunitions;
}



double Weapon::TotalLifetime() const
{
	if(totalLifetime < 0.)
	{
		totalLifetime = 0.;
		for(const auto &it : submunitions)
			totalLifetime = max(totalLifetime, it.first->TotalLifetime());
		totalLifetime += lifetime;
	}
	return totalLifetime;
}



double Weapon::Range() const
{
	return Velocity() * TotalLifetime();
}



// Legacy support: allow turret outfits with no turn rate to specify a
// default turnrate.
void Weapon::SetTurretTurn(double rate)
{
	turretTurn = rate;
}



double Weapon::TotalDamage(int index) const
{
	if(!calculatedDamage[index])
	{
		calculatedDamage[index] = true;
		for(const auto &it : submunitions)
			damage[index] += it.first->TotalDamage(index) * it.second;
	}
	return damage[index];
}
