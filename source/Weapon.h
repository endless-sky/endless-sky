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

#include "Body.h"

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
	const Body &WeaponSprite() const;
	const Sound *WeaponSound() const;
	const Outfit *Ammo() const;
	const Sprite *Icon() const;
	
	// Effects to be created at the start or end of the weapon's lifetime.
	const std::map<const Effect *, int> &FireEffects() const;
	const std::map<const Effect *, int> &LiveEffects() const;
	const std::map<const Effect *, int> &HitEffects() const;
	const std::map<const Effect *, int> &DieEffects() const;
	const std::map<const Outfit *, int> &Submunitions() const;
	
	// Accessor functions for various attributes.
	int Lifetime() const;
	int RandomLifetime() const;
	double Reload() const;
	double BurstReload() const;
	int BurstCount() const;
	int Homing() const;
	
	int MissileStrength() const;
	int AntiMissile() const;
	// Weapons of the same type will alternate firing (streaming) rather than
	// firing all at once (clustering) if the weapon is not an anti-missile and
	// is not vulnerable to anti-missile, or has the "stream" attribute.
	bool IsStreamed() const;
	
	double Velocity() const;
	double RandomVelocity() const;
	double Acceleration() const;
	double Drag() const;
	
	double Turn() const;
	double Inaccuracy() const;
	
	double Tracking() const;
	double OpticalTracking() const;
	double InfraredTracking() const;
	double RadarTracking() const;
	
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
	double DisruptionDamage() const;
	double SlowingDamage() const;
	double FuelDamage() const;
	
	double Piercing() const;
	
	double TotalLifetime() const;
	double Range() const;
	
	
private:
	double TotalDamage(int index) const;
	
	
private:
	// Sprites and sounds.
	Body sprite;
	const Sound *sound = nullptr;
	const Outfit *ammo = nullptr;
	const Sprite *icon = nullptr;
	
	// Fire, die and hit effects.
	std::map<const Effect *, int> fireEffects;
	std::map<const Effect *, int> liveEffects;
	std::map<const Effect *, int> hitEffects;
	std::map<const Effect *, int> dieEffects;
	std::map<const Outfit *, int> submunitions;
	
	// This stores whether or not the weapon has been loaded.
	bool isWeapon = false;
	bool isStreamed = false;
	
	// Attributes.
	int lifetime = 0;
	int randomLifetime = 0;
	double reload = 1.;
	double burstReload = 1.;
	int burstCount = 1;
	int homing = 0;
	
	int missileStrength = 0;
	int antiMissile = 0;
	
	double velocity = 0.;
	double randomVelocity = 0.;
	double acceleration = 0.;
	double drag = 0.;
	
	double turn = 0.;
	double inaccuracy = 0.;
	
	double tracking = 0.;
	double opticalTracking = 0.;
	double infraredTracking = 0.;
	double radarTracking = 0.;
	
	double firingEnergy = 0.;
	double firingForce = 0.;
	double firingFuel = 0.;
	double firingHeat = 0.;
	
	double splitRange = 0.;
	double triggerRadius = 0.;
	double blastRadius = 0.;
	double hitForce = 0.;
	
	static const int SHIELD_DAMAGE = 0;
	static const int HULL_DAMAGE = 1;
	static const int HEAT_DAMAGE = 2;
	static const int ION_DAMAGE = 3;
	static const int DISRUPTION_DAMAGE = 4;
	static const int SLOWING_DAMAGE = 5;
	static const int FUEL_DAMAGE = 6;
	mutable double damage[7] = {0., 0., 0., 0., 0., 0., 0.};
	
	double piercing = 0.;
	
	// Cache the calculation of these values, for faster access.
	mutable bool calculatedDamage[7] = {false, false, false, false, false, false, false};
	mutable double totalLifetime = -1.;
};



// Inline the accessors because they get called so frequently.
inline int Weapon::Lifetime() const { return lifetime; }
inline int Weapon::RandomLifetime() const { return randomLifetime; }
inline double Weapon::Reload() const { return reload; }
inline double Weapon::BurstReload() const { return burstReload; }
inline int Weapon::BurstCount() const { return burstCount; }
inline int Weapon::Homing() const { return homing; }

inline int Weapon::MissileStrength() const { return missileStrength; }
inline int Weapon::AntiMissile() const { return antiMissile; }
inline bool Weapon::IsStreamed() const { return isStreamed; }

inline double Weapon::Velocity() const { return velocity; }
inline double Weapon::RandomVelocity() const { return randomVelocity; }
inline double Weapon::Acceleration() const { return acceleration; }
inline double Weapon::Drag() const { return drag; }

inline double Weapon::Turn() const { return turn; }
inline double Weapon::Inaccuracy() const { return inaccuracy; }

inline double Weapon::Tracking() const { return tracking; }
inline double Weapon::OpticalTracking() const { return opticalTracking; }
inline double Weapon::InfraredTracking() const { return infraredTracking; }
inline double Weapon::RadarTracking() const { return radarTracking; }

inline double Weapon::FiringEnergy() const { return firingEnergy; }
inline double Weapon::FiringForce() const { return firingForce; }
inline double Weapon::FiringFuel() const { return firingFuel; }
inline double Weapon::FiringHeat() const { return firingHeat; }

inline double Weapon::Piercing() const { return piercing; }

inline double Weapon::SplitRange() const { return splitRange; }
inline double Weapon::TriggerRadius() const { return triggerRadius; }
inline double Weapon::BlastRadius() const { return blastRadius; }
inline double Weapon::HitForce() const { return hitForce; }

inline double Weapon::ShieldDamage() const { return TotalDamage(SHIELD_DAMAGE); }
inline double Weapon::HullDamage() const { return TotalDamage(HULL_DAMAGE); }
inline double Weapon::HeatDamage() const { return TotalDamage(HEAT_DAMAGE); }
inline double Weapon::IonDamage() const { return TotalDamage(ION_DAMAGE); }
inline double Weapon::DisruptionDamage() const { return TotalDamage(DISRUPTION_DAMAGE); }
inline double Weapon::SlowingDamage() const { return TotalDamage(SLOWING_DAMAGE); }
inline double Weapon::FuelDamage() const { return TotalDamage(FUEL_DAMAGE);}



#endif
