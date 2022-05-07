/* DamageDealt.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DAMAGE_DEALT_H_
#define DAMAGE_DEALT_H_

#include "EnergyLevels.h"
#include "Point.h"

class Weapon;



// A class representing the exact damage dealt to a ship for all
// damage types, passed to Ship so that it can be applied. Includes
// the weapon used, damage scale, and whether the damage was from a
// blast for Ship::TakeDamage to access.
class DamageDealt {
public:
	DamageDealt(const Weapon &weapon, double scaling)
		: weapon(weapon), scaling(scaling) {}

	// The weapon that dealt damage.
	const Weapon &GetWeapon() const noexcept;
	// The damage scaling that was used for this damage.
	double Scaling() const noexcept;

	// The levels of damage dealt.
	const EnergyLevels &Levels() const noexcept;
	// Hit force applied as a point vector.
	const Point &HitForce() const noexcept;


private:
	// Friend of DamageProfile so that it can easily set all the damage
	// values.
	friend class DamageProfile;

	const Weapon &weapon;
	double scaling;

	EnergyLevels levels;
	Point forcePoint;
};

inline const Weapon &DamageDealt::GetWeapon() const noexcept { return weapon; }
inline double DamageDealt::Scaling() const noexcept { return scaling; }

inline const EnergyLevels &DamageDealt::Levels() const noexcept { return levels; }
inline const Point &DamageDealt::HitForce() const noexcept { return forcePoint; }

#endif
