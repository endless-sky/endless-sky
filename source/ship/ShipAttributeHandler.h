/* ShipResourceHandler.h
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#pragma once

#include "ResourceLevels.h"

class Outfit;
class Ship;
class Weapon;



// A class which handles various aspects of a ship's resource levels,
// including taking damage, doing repairs, and calculating fractional
// thrust or turn values.
class ShipAttributeHandler {
public:
	enum class CanFireResult {
		NO_AMMO,
		NO_ENERGY,
		NO_FUEL,
		NO_HULL,
		NO_HEAT,
		NO_CORROSION,
		NO_DISCHARGE,
		NO_ION,
		NO_SCRAMBLING,
		NO_DISRUPTION,
		NO_SLOWING,
		NO_BURNING,
		NO_LEAKAGE,
		CAN_FIRE
	};


public:
	// Update the stored ResourceLevels for various actions a
	// ship can take (e.g. regenerating shields, thrusting).
	void Update(const Outfit &attributes);

	// Prevent various levels from going outside of allowed values
	// (e.g. energy being negative, shield being above capacity).
	void Clamp(ResourceLevels &input) const;
	// Clear all levels of input and set hull to -1.
	void Kill(ResourceLevels &input) const;
	// Clear the damage over time levels of the input.
	void ClearDoT(ResourceLevels &input) const;

	// Repair the given stat up to the maximum given the energy input and cost.
	// Updates the available variable with the remaining amount of repairs that
	// can be done.
	void DoRepair(double &stat, double &available, double maximum, ResourceLevels &input, const ResourceLevels &cost) const;
	// Apply status effects and DoT resistances to the input.
	void DoStatusEffects(ResourceLevels &input, bool disabled) const;

	// Return true if the given input has the energy to expend on the cost. Does
	// not check DoT levels.
	bool CanExpendBasic(const ResourceLevels &input, const ResourceLevels &cost) const;
	// Return true if the given input has the energy to expend on the entire cost.
	bool CanExpend(const ResourceLevels &input, const ResourceLevels &cost) const;
	// Return true if the given input has the energy to expend on the firing cost.
	// This ignores any shield costs, allowing ships to fire a weapon even with
	// no shields. This also prevents a ship from disabling itself as a result
	// of any firing hull cost.
	CanFireResult CanFire(const ResourceLevels &input, const ResourceLevels &cost) const;

	// Return the fraction of 100% output that the input can manage given the cost.
	double FractionalUsage(const ResourceLevels &input, const ResourceLevels &cost) const;

	// Apply damage * scale to the input. Hull, shields, energy, and fuel
	// are subtracted from input while all other levels are added to input.
	// Does not apply damage to DoT levels.
	void DamageBasic(ResourceLevels &input, const ResourceLevels &damage, double scale = 1.) const;
	// Applies damage to all levels.
	void Damage(ResourceLevels &input, const ResourceLevels &damage, double scale = 1.) const;

	// Construct an ResourceLevels object for the firing cost of the given weapon
	// when fired from the given ship.
	ResourceLevels FiringCost(const Weapon &weapon, const Ship &ship) const;


private:
	// Update the stored capacity for various ResourceLevels on a ship.
	void Capacity(const Outfit &attributes);

	// Update the stored ResourceLevels for each action a ship can take.
	void HullRepair(const Outfit &attributes);
	void ShieldRegen(const Outfit &attributes);

	void CorrosionResist(const Outfit &attributes);
	void DischargeResist(const Outfit &attributes);
	void IonizationResist(const Outfit &attributes);
	void ScramblingResist(const Outfit &attributes);
	void BurnResist(const Outfit &attributes);
	void LeakageResist(const Outfit &attributes);
	void DisruptionResist(const Outfit &attributes);
	void SlownessResist(const Outfit &attributes);

	void Thrust(const Outfit &attributes);
	void Turn(const Outfit &attributes);
	void ReverseThrust(const Outfit &attributes);
	void AfterburnerThrust(const Outfit &attributes);

	void Cloak(const Outfit &attributes);


private:
	// A ship can freely access the ResourceLevels of its own handler.
	friend class Ship;

	ResourceLevels capacity;

	ResourceLevels hullRepair;
	ResourceLevels hullRepairNoDelay;
	ResourceLevels shieldRegen;
	ResourceLevels shieldRegenNoDelay;

	ResourceLevels corrosionResist;
	ResourceLevels dischargeResist;
	ResourceLevels ionizationResist;
	ResourceLevels scramblingResist;
	ResourceLevels burnResist;
	ResourceLevels leakageResist;
	ResourceLevels disruptionResist;
	ResourceLevels slownessResist;

	ResourceLevels thrust;
	ResourceLevels turn;
	ResourceLevels reverseThrust;
	ResourceLevels afterburnerThrust;

	ResourceLevels cloak;
};
