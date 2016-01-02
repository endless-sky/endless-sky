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
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "sprite" && child.Size() >= 2)
			sprite.Load(child);
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
		else if(child.Size() >= 2)
		{
			if(child.Token(0) == "lifetime")
				lifetime = child.Value(1);
			else if(child.Token(0) == "reload")
				reload = child.Value(1);
			else if(child.Token(0) == "homing")
				homing = child.Value(1);
			else if(child.Token(0) == "missile strength")
				missileStrength = child.Value(1);
			else if(child.Token(0) == "anti-missile")
				antiMissile = child.Value(1);
			else if(child.Token(0) == "velocity")
				velocity = child.Value(1);
			else if(child.Token(0) == "acceleration")
				acceleration = child.Value(1);
			else if(child.Token(0) == "drag")
				drag = child.Value(1);
			else if(child.Token(0) == "turn")
				turn = child.Value(1);
			else if(child.Token(0) == "inaccuracy")
				inaccuracy = child.Value(1);
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
				shieldDamage = child.Value(1);
			else if(child.Token(0) == "hull damage")
				hullDamage = child.Value(1);
			else if(child.Token(0) == "heat damage")
				heatDamage = child.Value(1);
			else if(child.Token(0) == "ion damage")
				ionDamage = child.Value(1);
			else if(child.Token(0) == "hit force")
				hitForce = child.Value(1);
			else
				child.PrintTrace("Unrecognized weapon attribute: \"" + child.Token(0) + "\":");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool Weapon::IsWeapon() const
{
	return isWeapon;
}



// Get assets used by this weapon.
const Animation &Weapon::WeaponSprite() const
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



// These accessors cache the results of recursive calculations:
double Weapon::ShieldDamage() const
{
	if(totalShieldDamage < 0.)
	{
		totalShieldDamage = shieldDamage;
		for(const auto &it : submunitions)
			totalShieldDamage += it.first->ShieldDamage() * it.second;
	}
	return totalShieldDamage;
}



double Weapon::HullDamage() const
{
	if(totalHullDamage < 0.)
	{
		totalHullDamage = hullDamage;
		for(const auto &it : submunitions)
			totalHullDamage += it.first->HullDamage() * it.second;
	}
	return totalHullDamage;
}



double Weapon::HeatDamage() const
{
	if(totalHeatDamage < 0.)
	{
		totalHeatDamage = heatDamage;
		for(const auto &it : submunitions)
			totalHeatDamage += it.first->HeatDamage() * it.second;
	}
	return totalHeatDamage;
}




double Weapon::IonDamage() const
{
	if(totalIonDamage < 0.)
	{
		totalIonDamage = ionDamage;
		for(const auto &it : submunitions)
			totalIonDamage += it.first->IonDamage() * it.second;
	}
	return totalIonDamage;
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
