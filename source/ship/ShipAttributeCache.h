/* ShipAttributeCache.h
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



// A class for caching various commonly accessed attributes in Ship.
class ShipAttributeCache {
public:
	void Calibrate(const Ship &ship);


private:
	void Capacity(const Outfit &attributes, const Outfit &baseAttributes);
	void EnergyAndFuelGeneration(const Outfit &attributes);
	void HeatAndCooling(const Outfit &attributes);

	void HullRepair(const Outfit &attributes);
	void ShieldRegen(const Outfit &attributes);
	void Recovery(const Outfit &attributes);

	void Thrust(const Outfit &attributes);
	void Turn(const Outfit &attributes);
	void ReverseThrust(const Outfit &attributes);
	void AfterburnerThrust(const Outfit &attributes);

	void Cloaking(const Outfit &attributes);
	void Scanning(const Outfit &attributes);
	void Damage(const Outfit &attributes);
	void Misc(const Outfit &attributes);


private:
	// A ship can freely access its own attribute cache.
	friend class Ship;

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
	double heatCapacity = 0.;
	double overheatDamageThreshold = 1.;
	double overheatDamageRate = 0.;

	double cooling = 0.;
	double activeCooling = 0.;
	double coolingEnergy = 0.;
	double coolingInefficiency = 1.;

	int repairDelay = 0;
	int disabledRepairDelay = 0;
	double hullRepairRate = 0.;
	ResourceLevels hullRepairCost;
	double hullRepairRateWithDelay = 0.;
	ResourceLevels hullRepairWithDelayCost;

	int shieldDelay = 0;
	int depletedShieldDelay = 0;
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

	ResourceLevels damageProtection;
	double piercingProtection = 1.;
	double piercingResistance = 0.;
	double highShieldPermeability = 0.;
	double lowShieldPermeability = 0.;
	double cloakedShieldPermeability = 0.;
	double cloakedHullProtection = 0.;
	double cloakedShieldProtection = 0.;
	double forceProtection = 1.;

	double drag = 1.;
	double dragReduction = 1.;
	double accelerationMult = 1.;
	double inertiaReduction = 1.;
	double turnMult = 1.;

	float landingSpeed = 0.f;
	bool silentJumps = false;
	double selfDestruct = 0.;

	double turretTurnMult = 1.;
};
