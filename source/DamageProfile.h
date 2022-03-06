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
#include "Weather.h"

class DamageDealt;
class Ship;
class Weapon;



// A class that calculates how much damage a ship should take given the ship's
// attributes and the weapon it was hit by for each damage type. Bundles the
// results of these calculations into a DamageDealt object.
class DamageProfile {
public:
	// Constructor for damage taken from a weapon projectile.
	DamageProfile(Projectile::ImpactInfo info);
	// Constructor for damage taken from a hazard.
	DamageProfile(Weather::ImpactInfo info);

	// Calculate the damage dealt to the given ship.
	DamageDealt CalculateDamage(const Ship &ship, bool ignoreBlast = false) const;


private:
	// Calculate the shared k and rSquared variables for
	// any ship hit by a blast.
	void CalculateBlast();
	// Determine the damage scale for the given ship.
	double Scale(double scale, const Ship &ship, bool blast) const;
	// Populate the given DamageDealt object with values.
	void PopulateDamage(DamageDealt &damage, const Ship &ship) const;


private:
	// The weapon that the dealt damage.
	const Weapon &weapon;
	// The position of the projectile or hazard.
	Point position;
	// Whether damage is applied as a blast.
	bool isBlast;
	// The scaling as received before calculating damage.
	double inputScaling = 1.;
	// Whether damage is applied from a hazard.
	bool isHazard = false;

	// Fields for caching blast radius calculation values
	// that are shared by all ships that this profile could
	// impact.
	double k = 0.;
	double rSquared = 0.;
};

#endif
