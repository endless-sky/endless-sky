/* HazardProfile.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef HAZARD_PROFILE_H_
#define HAZARD_PROFILE_H_

#include "DamageProfile.h"

#include "Projectile.h"

class Point;
class Ship;
class Weapon;


// A class that calculates how much damage a ship should take given the ship's
// attributes and the weapon it was hit by for each damage type.
class HazardProfile : public DamageProfile {
public:
	HazardProfile(const Projectile::ImpactInfo &info, double damageScaling, bool isBlast = false);
	
	virtual DamageDealt CalculateDamage(const Ship &ship) const override;


private:
	// Determine the damage scale for the given ship.
	virtual double Scale(double scale, const Ship &ship) const override;


private:
	// The weapon that the hazard deals damage with.
	const Weapon &weapon;
	// The origin of the hazard.
	const Point &position;
	// The scaling as recieved before calculating damage.
	double inputScaling;
	// Whether damage is applied as a blast.
	bool isBlast;

	// Precomputed blast damage values.
	double k;
	double rSquared;
};

#endif
