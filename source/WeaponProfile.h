/* WeaponProfile.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WEAPON_PROFILE_H_
#define WEAPON_PROFILE_H_

#include "DamageProfile.h"

#include "Projectile.h"

class Point;
class Ship;
class Weapon;



// A class representing damage created from a projectile that gets applied
// to a ship. Weapon damage differs from other damage in that whether
// the damage gets applied as a blast depends on whether the weapon
// is a blast radius weapon and the ship impacted was not directly hit.
class WeaponProfile : public DamageProfile {
public:
	WeaponProfile(const Projectile::ImpactInfo &info, bool isBlast = false);

	// Set whether blast damage is applied on the next CalculateDamage call.
	void SetBlast(bool blast);

	// Calculate the damage dealt to the given ship.
	virtual DamageDealt CalculateDamage(const Ship &ship) const override;


private:
	// Determine the damage scale for the given ship.
	virtual double Scale(double scale, const Ship &ship) const override;


private:
	// The weapon that the projectile deals damage with.
	const Weapon &weapon;
	// The position of the projectile.
	const Point &position;
	// The scaling as recieved before calculating damage.
	double inputScaling = 1.;
	// Whether damage is applied as a blast.
	bool isBlast;

	// Fields for caching blast radius calculation values
	// that are shared by all ships that this profile could
	// impact.
	double k;
	double rSquared;
};

#endif
