/* DamageProfile.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DAMAGE_PROFILE_H_
#define DAMAGE_PROFILE_H_

#include "Point.h"
#include "Projectile.h"

#include <string>

class Outfit;
class Ship;
class Weapon;


// A class that calculates how much damage a ship should take given the ship's
// attributes and the weapon it was hit by for each damage type.
class DamageProfile {
public:
	DamageProfile(const Ship &ship, const Projectile::ImpactInfo &info, double damageScaling, bool isBlast = false);

	// Calculate the damage dealt to the ship given its current shield and disruption levels.
	void CalculateDamage(double shields, double disrupted);

	const Weapon &GetWeapon() const;
	double Scaling() const;
	const Point &Position() const;
	bool IsBlast() const;

	// Instantaneous damage types.
	double Shield() const;
	double Hull() const;
	double Energy() const;
	double Heat() const;
	double Fuel() const;

	// DoT damage types with an instantaneous analog.
	double Discharge() const;
	double Corrosion() const;
	double Ion() const;
	double Burn() const;
	double Leak() const;

	// Unique special damage types.
	double Disruption() const;
	double Slowing() const;

	// Hit force applied as a point vector.
	const Point &HitForce() const;


private:
	// Return the damage scale that a damage type should use given the
	// default percentage that is blocked by shields and the name of
	// its protection attribute.
	double ScaleType(double blocked, const char *protectionAttribute) const;


private:
	const Ship &ship;
	const Outfit &attributes;
	const Weapon &weapon;
	const Point &position;
	double distanceTraveled;
	double scaling;
	bool isBlast;
	
	double shieldFraction;

	double hullDamage = 0.;
	double shieldDamage = 0.;
	double energyDamage = 0.;
	double heatDamage = 0.;
	double fuelDamage = 0.;

	double corrosionDamage = 0.;
	double dischargeDamage = 0.;
	double ionDamage = 0.;
	double burnDamage = 0.;
	double leakDamage = 0.;

	double disruptionDamage = 0.;
	double slowingDamage = 0.;

	double hitForce = 0.;
	Point forcePoint;
};

inline double DamageProfile::Shield() const { return shieldDamage; }
inline double DamageProfile::Hull() const { return hullDamage; }
inline double DamageProfile::Energy() const { return energyDamage; }
inline double DamageProfile::Heat() const { return heatDamage; }
inline double DamageProfile::Fuel() const { return fuelDamage; }

inline double DamageProfile::Discharge() const { return dischargeDamage; }
inline double DamageProfile::Corrosion() const { return corrosionDamage; }
inline double DamageProfile::Ion() const { return ionDamage; }
inline double DamageProfile::Burn() const { return burnDamage; }
inline double DamageProfile::Leak() const { return leakDamage; }

inline double DamageProfile::Disruption() const { return disruptionDamage; }
inline double DamageProfile::Slowing() const { return slowingDamage; }

inline const Point &DamageProfile::HitForce() const { return forcePoint; }

#endif
