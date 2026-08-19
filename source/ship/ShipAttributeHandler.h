/* ShipAttributeHandler.h
Copyright (c) 2025 by Amazinite

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

#include "ResourceLevels.h"

class Outfit;
class Ship;
class Weapon;



// A class which handles various aspects of a ship's resource levels,
// including taking damage, doing repairs, and calculating fractional
// thrust or turn values.
class ShipAttributeHandler {
public:
	// Setup this attribute handler with a pointer to the ship instance that it is handling attributes for.
	void Setup(Ship *parent);

	// Update the stored ResourceLevels for various actions a
	// ship can take (e.g. regenerating shields, thrusting).
	void Calibrate();

	// Functions for classes outside of Ship to get the cached values
	// and determine what this ship is capable of.
	double CargoScanPower() const;
	double OutfitScanPower() const;
	double AsteroidScanPower() const;
	double AtmosphereScan() const;
	bool Inscrutable() const;
	bool CanCommunicateWhileCloaked() const;

	double ReverseThrust() const;
	double AfterburnerThrust() const;
	bool ShouldUseAfterburner(const ResourceLevels &available) const;

	bool SilentJumps() const;

	double CloakFuelCost() const;
	bool HasFuelForCloak(const ResourceLevels &available) const;
	bool CanRecoverHullWhileCloaked() const;
	bool CanRecoverShieldsWhileCloaked() const;

	double TurretTurnMultiplier() const;

	const ResourceLevels &DamageProtection() const;
	double PiercingProtection() const;
	double PiercingResistance() const;
	double HighShieldPermeability() const;
	double LowShieldPermeability() const;
	double CloakedShieldPermeability() const;
	double CloakedHullProtection() const;
	double CloakedShieldProtection() const;
	double ForceProtection() const;


private:
	// Update the stored capacity for various ResourceLevels on a ship.
	void Capacity();
	void EnergyAndFuelGeneration();
	void HeatAndCooling();

	// Update the stored ResourceLevels for each action a ship can take.
	void HullRepair();
	void ShieldRegen();
	void Recovery();

	void Thrust();
	void Turn();
	void ReverseThrust();
	void AfterburnerThrust();

	void Cloaking();
	void Scanning();
	void Misc();


private:
	// A ship can freely access the ResourceLevels of its own handler.
	friend class Ship;

	Ship *ship = nullptr;
	const Outfit *attributes = nullptr;

	double outfitCapacity = 0.;
	double weaponCapacity = 0.;
	double engineCapacity = 0.;
	double cargoSpace = 0.;
	bool automaton = false;
	int requiredCrew = 0;
	int mandatoryCrew = 0;
	int bunks = 0;
	int crewEquiv = 0;
	bool onlyUseCrewEquiv = false;

	double energyGeneration = 0.;
	double energyConsumption = 0.;

	double fuelGeneration = 0.;
	double fuelConsumption = 0.;
	double fuelEnergy = 0.;
	double fuelHeat = 0.;

	double ramscoop = 0.;
	double solarCollection = 0.;
	double solarHeat = 0.;

	double heatGeneration = 0.;
	double heatDissipation = 0.;
	double heatCapacity = 0.;

	double cooling = 0.;
	double activeCooling = 0.;
	double coolingEnergy = 0.;
	double coolingInefficiency = 1.;

	int repairDelay = 0;
	double hullRepairRate = 0.;
	ResourceLevels hullRepairCost;
	double hullRepairRateWithDelay = 0.;
	ResourceLevels hullRepairWithDelayCost;

	int depletedShieldDelay = 0;
	int shieldDelay = 0;
	double shieldRegenRate = 0.;
	ResourceLevels shieldRegenCost;
	double shieldRegenRateWithDelay = 0.;
	ResourceLevels shieldRegenWithDelayCost;

	int recoveryTime = 0;
	ResourceLevels recoveryCost;

	double thrust = 0.;
	ResourceLevels thrustCost;
	double turn = 0.;
	ResourceLevels turnCost;
	double reverseThrust = 0.;
	ResourceLevels reverseThrustCost;
	double afterburnerThrust = 0.;
	ResourceLevels afterburnerThrustCost;

	ResourceLevels cloakCost;
	double cloak = 0.;
	double cloakByMass = 0.;
	double cloakHullThreshold = 0.;
	double cloakingShieldDelay = 0.;
	double cloakingHullDelay = 0.;
	double cloakPhasing = 0.;
	double cloakedRepairMult = 0.;
	double cloakedRegenMult = 0.;
	double cloakedFiring = 0.;
	bool canBoardWhileCloaked = false;
	bool canAfterburnerWhileCloaked = false;
	bool canCommunicateWhileCloaked = false;
	bool canFireWhileCloaked = false;
	bool canPickupWhileCloaked = false;
	bool canScanWhileCloaked = false;
	bool canDeployWhileCloaked = false;

	double cargoScanPower = 0.;
	double outfitScanPower = 0.;
	double cargoScanSpeed = 0.;
	double outfitScanSpeed = 0.;
	double cargoScanOpacity = 0.;
	double outfitScanOpacity = 0.;
	double asteroidScanPower = 0.;
	double atmosphereScan = 0.;
	bool silentScans = false;
	bool inscrutable = false;

	double overheatDamageThreshold = 1.;
	double overheatDamageRate = 0.;

	double drag = 1.;
	double dragReduction = 1.;
	double accelerationMult = 1.;
	double inertiaReduction = 1.;
	double turnMult = 1.;

	float landingSpeed = 0.f;
	bool silentJumps = false;
	double selfDestruct = 0.;

	double turretTurnMult = 1.;
	ResourceLevels damageProtection;
	double piercingProtection = 1.;
	double piercingResistance = 0.;
	double highShieldPermeability = 0.;
	double lowShieldPermeability = 0.;
	double cloakedShieldPermeability = 0.;
	double cloakedHullProtection = 0.;
	double cloakedShieldProtection = 0.;
	double forceProtection = 1.;
};
