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


// A class that calculates how much damage a ship should take given the ship's
// attributes and the weapon it was hit by for each damage type.
class WeaponProfile : public DamageProfile {
public:
	WeaponProfile(const Projectile::ImpactInfo &info, bool isBlast = false);

	// Set whether blast damage is applied on the next CalculateDamage call.
	void SetBlast(bool blast);
	
	virtual DamageDealt CalculateDamage(const Ship &ship) const override;


private:
	// Finish any calculations that were started in the constructor.
	virtual void FinishPrecalculations(DamageDealt &damage, const Ship &ship) const override;


private:
	// The weapon that the projectile deals damage with.
	const Weapon &weapon;
	// The position of the projectile.
	const Point &position;
	// The distance that the projectile traveled or the distance from
	// the hazard origin.
	double distanceTraveled;
	// The scaling as recieved before calculating damage.
	double inputScaling;
	// Whether damage is applied as a blast.
	bool isBlast;

	// Precomputed blast damage values.
	double k;
	double rSquared;
};

#endif
