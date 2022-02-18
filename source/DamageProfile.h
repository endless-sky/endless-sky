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

class HazardProfile;
class Ship;
class Weapon;
class WeaponProfile;


// A class that calculates how much damage a ship should take given the ship's
// attributes and the weapon it was hit by for each damage type.
class DamageProfile {
public:
	class DamageDealt {
	public:
		DamageDealt(const Weapon &weapon, double scaling, bool isBlast)
			: weapon(weapon), scaling(scaling), isBlast(isBlast) {}

		// The weapon that dealt damage.
		const Weapon &GetWeapon() const { return weapon; };
		// The scaling that was used for this damage.
		double Scaling() const { return scaling; };
		// Whether damage was dealt as a blast.
		bool IsBlast() const { return isBlast; };

		// Instantaneous damage types.
		double Shield() const noexcept { return shieldDamage; }
		double Hull() const noexcept { return hullDamage; }
		double Energy() const noexcept { return energyDamage; }
		double Heat() const noexcept { return heatDamage; }
		double Fuel() const noexcept { return fuelDamage; }

		// DoT damage types with an instantaneous analog.
		double Discharge() const noexcept { return dischargeDamage; }
		double Corrosion() const noexcept { return corrosionDamage; }
		double Ion() const noexcept { return ionDamage; }
		double Burn() const noexcept { return burnDamage; }
		double Leak() const noexcept { return leakDamage; }

		// Unique special damage types.
		double Disruption() const noexcept { return disruptionDamage; }
		double Slowing() const noexcept { return slowingDamage; }

		// Hit force applied as a point vector.
		const Point &HitForce() const noexcept { return forcePoint; }


	private:
		friend class DamageProfile;
		friend class HazardProfile;
		friend class WeaponProfile;

		const Weapon &weapon;
		// The final scaling that gets applied to a ship, influenced by
		// the ship's attributes and whether this ship is treated differently
		// from other ships impacted by this DamageProfile.
		double scaling;
		bool isBlast;

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

		Point forcePoint;
	};

public:
	virtual ~DamageProfile() = default;

	// Calculate the damage dealt to the given ship.
	virtual DamageDealt CalculateDamage(const Ship &ship) const = 0;


protected:
	void PopulateDamage(DamageDealt &damage, const Ship &ship, const Point &position) const;


private:
	// Finish any calculations that were started in the constructor.
	virtual void FinishPrecalculations(DamageDealt &damage, const Ship &ship) const = 0;
};

#endif
