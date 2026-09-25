/* DamageProfile.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Point.h"
#include "Projectile.h"
#include "Weather.h"

class DamageDealt;
class Entity;
class Weapon;



// A class that calculates how much damage an entity should take given the entity's
// attributes and the weapon it was hit by for each damage type. Bundles the
// results of these calculations into a DamageDealt object.
class DamageProfile {
public:
	// Constructor for damage taken from a weapon projectile.
	explicit DamageProfile(const Entity &entity, const Projectile::ImpactInfo &info, bool ignoreBlast = false);
	// Constructor for damage taken from a hazard.
	explicit DamageProfile(const Entity &entity, const Weather::ImpactInfo &info, bool ignoreBlast = false);
	// Constructor for damage taken from a weapon with no physical source, such as from
	// an NPC's placement criteria.
	explicit DamageProfile(const Entity &entity, const Weapon &weapon);

	const Weapon &GetWeapon() const;
	const Entity &GetEntity() const;
	double Scaling() const;

	// Calculate the damage dealt to the entity at this exact moment.
	DamageDealt CalculateDamage() const;


private:
	// Determine the damage scale against the tracked entity.
	void CalculateScaling();


private:
	// The entity that this profile is tracking damage against.
	const Entity *entity;
	// The weapon that dealt the damage.
	const Weapon *weapon;
	// The position of the projectile or hazard.
	Point position;
	// Whether damage is applied as a blast.
	bool isBlast;
	// The base scaling to apply against the weapon damage
	// before the entity's attributes are considered.
	double scaling = 1.;
	// Whether damage is applied from a hazard.
	bool isHazard = false;
};
