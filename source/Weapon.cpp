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
		if(child.Token(0) == "sprite" && child.Size() >= 2)
			sprite.LoadSprite(child);
		else if(child.Token(0) == "sound" && child.Size() >= 2)
			sound = Audio::Get(child.Token(1));
		else if(child.Token(0) == "ammo" && child.Size() >= 2)
			ammo = GameData::Outfits().Get(child.Token(1));
		else if(child.Token(0) == "icon" && child.Size() >= 2)
			icon = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "fire effect" && child.Size() >= 2)
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			fireEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(child.Token(0) == "live effect" && child.Size() >= 2)
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			liveEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(child.Token(0) == "hit effect" && child.Size() >= 2)
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			hitEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(child.Token(0) == "die effect" && child.Size() >= 2)
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			dieEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(child.Token(0) == "submunition" && child.Size() >= 2)
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			submunitions[GameData::Outfits().Get(child.Token(1))] += count;
		}
		else if(child.Token(0) == "stream")
			isStreamed = true;
		else if(child.Token(0) == "cluster")
			isClustered = true;
		else if(child.Size() >= 2)
		{
			if(child.Token(0) == "lifetime")
				lifetime = max(0., child.Value(1));
			else if(child.Token(0) == "random lifetime")
				randomLifetime = max(0., child.Value(1));
			else if(child.Token(0) == "reload")
				reload = max(1., child.Value(1));
			else if(child.Token(0) == "burst reload")
				burstReload = max(1., child.Value(1));
			else if(child.Token(0) == "burst count")
				burstCount = max(1., child.Value(1));
			else if(child.Token(0) == "homing")
				homing = child.Value(1);
			else if(child.Token(0) == "missile strength")
				missileStrength = max(0., child.Value(1));
			else if(child.Token(0) == "anti-missile")
				antiMissile = max(0., child.Value(1));
			else if(child.Token(0) == "velocity")
				velocity = child.Value(1);
			else if(child.Token(0) == "random velocity")
				randomVelocity = child.Value(1);
			else if(child.Token(0) == "acceleration")
				acceleration = child.Value(1);
			else if(child.Token(0) == "drag")
				drag = child.Value(1);
			else if(child.Token(0) == "turn")
				turn = child.Value(1);
			else if(child.Token(0) == "inaccuracy")
				inaccuracy = child.Value(1);
			else if(child.Token(0) == "tracking")
				tracking = max(0., min(1., child.Value(1)));
			else if(child.Token(0) == "optical tracking")
				opticalTracking = max(0., min(1., child.Value(1)));
			else if(child.Token(0) == "infrared tracking")
				infraredTracking = max(0., min(1., child.Value(1)));
			else if(child.Token(0) == "radar tracking")
				radarTracking = max(0., min(1., child.Value(1)));
			else if(child.Token(0) == "firing energy")
				firingEnergy = child.Value(1);
			else if(child.Token(0) == "firing force")
				firingForce = child.Value(1);
			else if(child.Token(0) == "firing fuel")
				firingFuel = child.Value(1);
			else if(child.Token(0) == "firing heat")
				firingHeat = child.Value(1);
			else if(child.Token(0) == "split range")
				splitRange = child.Value(1);
			else if(child.Token(0) == "trigger radius")
				triggerRadius = child.Value(1);
			else if(child.Token(0) == "blast radius")
				blastRadius = child.Value(1);
			else if(child.Token(0) == "shield damage")
				damage[SHIELD_DAMAGE] = child.Value(1);
			else if(child.Token(0) == "hull damage")
				damage[HULL_DAMAGE] = child.Value(1);
			else if(child.Token(0) == "heat damage")
				damage[HEAT_DAMAGE] = child.Value(1);
			else if(child.Token(0) == "ion damage")
				damage[ION_DAMAGE] = child.Value(1);
			else if(child.Token(0) == "disruption damage")
				damage[DISRUPTION_DAMAGE] = child.Value(1);
			else if(child.Token(0) == "slowing damage")
				damage[SLOWING_DAMAGE] = child.Value(1);
			else if(child.Token(0) == "fuel damage")
				damage[FUEL_DAMAGE] = child.Value(1);
			else if(child.Token(0) == "hit force")
				hitForce = child.Value(1);
			else if(child.Token(0) == "piercing")
				piercing = max(0., min(1., child.Value(1)));
			else
				child.PrintTrace("Unrecognized weapon attribute: \"" + child.Token(0) + "\":");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
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
