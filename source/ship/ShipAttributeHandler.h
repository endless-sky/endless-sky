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
	// Setup this attribute handler with a pointer to the ship instance that it is handling attributes for.
	void Setup(const Ship *parent, ResourceLevels *levels);

	// Update the stored ResourceLevels for various actions a
	// ship can take (e.g. regenerating shields, thrusting).
	void Calibrate();

	// Clear all levels and set hull to -1.
	void Kill() const;
	// Clear the damage over time levels.
	void ClearDoT() const;

	// Repair the given stat up to the maximum that the ship is capable of given the cost.
	// Updates the available variable with the remaining amount of repairs that
	// can be done.
	void DoRepair(double &stat, double &available, double maximum, const ResourceLevels &cost) const;
	// Apply status effects and DoT resistances to the ship.
	void DoStatusEffects(bool disabled) const;

	// Return true if the ship has the resources to expend on the entire cost.
	bool CanExpend(const ResourceLevels &cost) const;
	// Return the fraction of 100% output that the ship can manage given the cost.
	double FractionalUsage(const ResourceLevels &cost) const;

	// Construct a ResourceLevels object for the firing cost of the given weapon
	// when fired from the given ship.
	ResourceLevels FiringCost(const Weapon &weapon) const;
	// Return true if the ship has the resources to expend on the firing cost.
	// This ignores any shield costs, allowing ships to fire a weapon even with
	// no shields. This also prevents a ship from disabling itself as a result
	// of any firing hull cost.
	CanFireResult CanFire(const Weapon &weapon) const;

	// Apply damage * scale to the ship. Hull, shields, energy, and fuel
	// are subtracted from the resources while all other levels are added.
	void Damage(const ResourceLevels &damage, double scale = 1.) const;


private:
	// Update the stored capacity for various ResourceLevels on a ship.
	void Capacity();

	// Update the stored ResourceLevels for each action a ship can take.
	void HullRepair();
	void ShieldRegen();

	void CorrosionResist();
	void DischargeResist();
	void IonizationResist();
	void ScramblingResist();
	void BurnResist();
	void LeakageResist();
	void DisruptionResist();
	void SlownessResist();

	void Thrust();
	void Turn();
	void ReverseThrust();
	void AfterburnerThrust();


private:
	// A ship can freely access the ResourceLevels of its own handler.
	friend class Ship;

	// TODO: Preferably, these would be references provided when this class is constructed
	// instead of pointers provided by the Setup function. Unfortunately, we can't populate
	// references at construction time since the Ship class represents both templates and instances
	// of a ship. If ever we have separate classes for Ship templates and Ship instances, then these
	// could be changed to be populated at construction when a Ship instance is constructed.
	// For now, the Setup function is called in Ship::FinishLoading, where we know that the Ship is
	// now an instance whose pointer we can trust.
	const Ship *ship = nullptr;
	const Outfit *attributes = nullptr;
	ResourceLevels *shipLevels = nullptr;

	ResourceLevels capacity;
	double minimumHull;

	double hullRepairRate;
	ResourceLevels hullRepairCost;
	double hullRepairRateWithDelay;
	ResourceLevels hullRepairWithDelayCost;
	double shieldRegenRate;
	ResourceLevels shieldRegenCost;
	double shieldRegenRateWithDelay;
	ResourceLevels shieldRegenWithDelayCost;

	double corrosionResistance;
	ResourceLevels corrosionResistCost;
	double dischargeResistance;
	ResourceLevels dischargeResistCost;
	double ionizationResistance;
	ResourceLevels ionizationResistCost;
	double scramblingResistance;
	ResourceLevels scramblingResistCost;
	double burnResistance;
	ResourceLevels burnResistCost;
	double leakResistance;
	ResourceLevels leakageResistCost;
	double disruptionResistance;
	ResourceLevels disruptionResistCost;
	double slowingResistance;
	ResourceLevels slownessResistCost;

	double thrust;
	ResourceLevels thrustCost;
	double turn;
	ResourceLevels turnCost;
	double reverseThrust;
	ResourceLevels reverseThrustCost;
	double afterburnerThrust;
	ResourceLevels afterburnerThrustCost;
};
