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
	const Weapon &GetWeapon() const;
	// The damage scaling that was used for this damage.
	double Scaling() const;

	// Instantaneous damage types.
	double Shield() const noexcept;
	double Hull() const noexcept;
	double Energy() const noexcept;
	double Heat() const noexcept;
	double Fuel() const noexcept;

	// DoT damage types with an instantaneous analog.
	double Discharge() const noexcept;
	double Corrosion() const noexcept;
	double Ion() const noexcept;
	double Scrambling() const noexcept;
	double Burn() const noexcept;
	double Leak() const noexcept;

	// Unique special damage types.
	double Disruption() const noexcept;
	double Slowing() const noexcept;

	// Hit force applied as a point vector.
	const Point &HitForce() const noexcept;


private:
	// Friend of DamageProfile so that it can easily set all the damage
	// values.
	friend class DamageProfile;

	const Weapon &weapon;
	double scaling;

	double hullDamage = 0.;
	double shieldDamage = 0.;
	double energyDamage = 0.;
	double heatDamage = 0.;
	double fuelDamage = 0.;

	double corrosionDamage = 0.;
	double dischargeDamage = 0.;
	double ionDamage = 0.;
	double scramblingDamage = 0.;
	double burnDamage = 0.;
	double leakDamage = 0.;

	double disruptionDamage = 0.;
	double slowingDamage = 0.;

	Point forcePoint;
};

inline const Weapon &DamageDealt::GetWeapon() const { return weapon; }
inline double DamageDealt::Scaling() const { return scaling; }

inline double DamageDealt::Shield() const noexcept { return shieldDamage; }
inline double DamageDealt::Hull() const noexcept { return hullDamage; }
inline double DamageDealt::Energy() const noexcept { return energyDamage; }
inline double DamageDealt::Heat() const noexcept { return heatDamage; }
inline double DamageDealt::Fuel() const noexcept { return fuelDamage; }

inline double DamageDealt::Discharge() const noexcept { return dischargeDamage; }
inline double DamageDealt::Corrosion() const noexcept { return corrosionDamage; }
inline double DamageDealt::Ion() const noexcept { return ionDamage; }
inline double DamageDealt::Scrambling() const noexcept { return scramblingDamage; }
inline double DamageDealt::Burn() const noexcept { return burnDamage; }
inline double DamageDealt::Leak() const noexcept { return leakDamage; }

inline double DamageDealt::Disruption() const noexcept { return disruptionDamage; }
inline double DamageDealt::Slowing() const noexcept { return slowingDamage; }

inline const Point &DamageDealt::HitForce() const noexcept { return forcePoint; }
