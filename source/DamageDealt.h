/* DamageDealt.h
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
#include "ship/ResourceLevels.h"

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

	// The damage dealt to resource levels.
	const ResourceLevels &Levels() const noexcept;
	// Hit force applied as a point vector.
	const Point &HitForce() const noexcept;


private:
	// Friend of DamageProfile so that it can easily set all the damage
	// values.
	friend class DamageProfile;

	const Weapon &weapon;
	double scaling;

	ResourceLevels levels;
	Point forcePoint;
};

inline const Weapon &DamageDealt::GetWeapon() const noexcept { return weapon; }
inline double DamageDealt::Scaling() const noexcept { return scaling; }

inline const ResourceLevels &DamageDealt::Levels() const noexcept { return levels; }
inline const Point &DamageDealt::HitForce() const noexcept { return forcePoint; }
