/* Weapon.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WEAPON_H_
#define WEAPON_H_

#include "Animation.h"

#include <map>

class DataNode;
class Effect;
class Outfit;
class Sound;
class Sprite;



// Class representing all the characteristics of a weapon, including sprites and
// effects, sounds, icons, ammo, submunitions, and other attributes. Storing
// these parameters in a separate class keeps each Projectile from needing a
// copy of them, and storing them as class variables instead of in a map of
// string to double significantly reduces access time.
class Weapon {
public:
	// Load from a "weapon" node, either in an outfit or in a ship (explosion).
	void LoadWeapon(const DataNode &node);
	bool IsWeapon() const;
	
	// Get assets used by this weapon.
	const Animation &WeaponSprite() const;
	const Sound *WeaponSound() const;
	const Outfit *Ammo() const;
	const Sprite *Icon() const;
	
	// Effects to be created at the start or end of the weapon's lifetime.
	const std::map<const Effect *, int> &FireEffects() const;
	const std::map<const Effect *, int> &HitEffects() const;
	const std::map<const Effect *, int> &DieEffects() const;
	const std::map<const Outfit *, int> &Submunitions() const;
	
	// Accessor functions for various attributes.
	int Lifetime() const;
	int Reload() const;
	int Homing() const;
	
	int MissileStrength() const;
	int AntiMissile() const;
	
	double Velocity() const;
	double Acceleration() const;
	double Drag() const;
	
	double Turn() const;
	double Inaccuracy() const;
	
	double FiringEnergy() const;
	double FiringForce() const;
	double FiringFuel() const;
	double FiringHeat() const;
	
	double SplitRange() const;
	double TriggerRadius() const;
	double BlastRadius() const;
	double HitForce() const;
	
	// These values include all submunitions:
	double ShieldDamage() const;
	double HullDamage() const;
	double HeatDamage() const;
	double IonDamage() const;
	
	double TotalLifetime() const;
	double Range() const;
	
	
private:
	// Sprites and sounds.
	Animation sprite;
	const Sound *sound = nullptr;
	const Outfit *ammo = nullptr;
	const Sprite *icon = nullptr;
	
	// Fire, die and hit effects.
	std::map<const Effect *, int> fireEffects;
	std::map<const Effect *, int> hitEffects;
	std::map<const Effect *, int> dieEffects;
	std::map<const Outfit *, int> submunitions;
	
	// This stores whether or not the weapon has been loaded.
	bool isWeapon = false;
	
	// Attributes.
	int lifetime = 0;
	int reload = 0;
	int homing = 0;
	
	int missileStrength = 0.;
	int antiMissile = 0.;
	
	double velocity = 0.;
	double acceleration = 0.;
	double drag = 0.;
	
	double turn = 0.;
	double inaccuracy = 0.;
	
	double firingEnergy = 0.;
	double firingForce = 0.;
	double firingFuel = 0.;
	double firingHeat = 0.;
	
	double splitRange = 0.;
	double triggerRadius = 0.;
	double blastRadius = 0.;
	
	double shieldDamage = 0.;
	double hullDamage = 0.;
	double heatDamage = 0.;
	double ionDamage = 0.;
	double hitForce = 0.;
	
	// Cache the calculation of these values, for faster access.
	mutable double totalShieldDamage = -1.;
	mutable double totalHullDamage = -1.;
	mutable double totalHeatDamage = -1.;
	mutable double totalIonDamage = -1.;
	mutable double totalLifetime = -1.;
};



// Inline the accessors because they get called so frequently.
inline int Weapon::Lifetime() const { return lifetime; }
inline int Weapon::Reload() const { return reload; }
inline int Weapon::Homing() const { return homing; }

inline int Weapon::MissileStrength() const { return missileStrength; }
inline int Weapon::AntiMissile() const { return antiMissile; }

inline double Weapon::Velocity() const { return velocity; }
inline double Weapon::Acceleration() const { return acceleration; }
inline double Weapon::Drag() const { return drag; }

inline double Weapon::Turn() const { return turn; }
inline double Weapon::Inaccuracy() const { return inaccuracy; }

inline double Weapon::FiringEnergy() const { return firingEnergy; }
inline double Weapon::FiringForce() const { return firingForce; }
inline double Weapon::FiringFuel() const { return firingFuel; }
inline double Weapon::FiringHeat() const { return firingHeat; }

inline double Weapon::SplitRange() const { return splitRange; }
inline double Weapon::TriggerRadius() const { return triggerRadius; }
inline double Weapon::BlastRadius() const { return blastRadius; }
inline double Weapon::HitForce() const { return hitForce; }



#endif
