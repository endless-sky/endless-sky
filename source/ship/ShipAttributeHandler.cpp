/* ShipAttributeHandler.cpp
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

#include "ShipAttributeHandler.h"

#include "../Outfit.h"
#include "../Ship.h"

#include <cmath>

using namespace std;



void ShipAttributeHandler::Setup(Ship *parent)
{
	ship = parent;
	attributes = &parent->Attributes();
}



void ShipAttributeHandler::Calibrate()
{
	// Basic behaviors:
	Capacity();
	EnergyAndFuelGeneration();
	HeatAndCooling();

	// Repairs:
	HullRepair();
	ShieldRegen();
	Recovery();

	// Movement:
	Thrust();
	Turn();
	ReverseThrust();
	AfterburnerThrust();

	// Miscellaneous actions and attributes:
	Cloaking();
	Scanning();
	Misc();

	// Entity-level attributes:
	ship->CacheAttributes();
}



double ShipAttributeHandler::CargoScanPower() const
{
	return cargoScanPower;
}



double ShipAttributeHandler::OutfitScanPower() const
{
	return outfitScanPower;
}



double ShipAttributeHandler::AsteroidScanPower() const
{
	return asteroidScanPower;
}



double ShipAttributeHandler::AtmosphereScan() const
{
	return atmosphereScan;
}



bool ShipAttributeHandler::Inscrutable() const
{
	return inscrutable;
}



bool ShipAttributeHandler::CanCommunicateWhileCloaked() const
{
	return canCommunicateWhileCloaked;
}



double ShipAttributeHandler::ReverseThrust() const
{
	return reverseThrust;
}



double ShipAttributeHandler::AfterburnerThrust() const
{
	return afterburnerThrust;
}



bool ShipAttributeHandler::ShouldUseAfterburner(const ResourceLevels &available) const
{
	double remainingFuel = available.fuel;
	double neededFuel = afterburnerThrustCost.fuel;
	double remainingEnergy = available.energy;
	double neededEnergy = afterburnerThrustCost.energy;
	// If there is no battery energy to use, consider how much energy might be produced this frame.
	// This is a lower-bound calculation that assumes that this ship is far from the system
	// center and is not gaining energy from anything aside from basic energy generation and
	// solar collection. (i.e. sources of energy like fuel consumption are ignored.)
	if(remainingEnergy == 0.)
		remainingEnergy = energyGeneration + 0.2 * solarCollection - energyConsumption;
	double outputHeat = afterburnerThrustCost.heat / (100. * ship->Mass());
	// Don't use an afterburner if it uses up more fuel than is needed to jump,
	// uses up more than 25% of our current energy reserves,
	// or pushes us over 90% of the way to being overheated.
	// TODO: Is this meant to prevent use if energy is below 25% of capacity?
	// Preventing use if the energy needed takes up more than 25% of the remaining energy is a bit odd.
	if((!neededFuel || remainingFuel - neededFuel > ship->JumpNavigation().JumpFuel())
			&& (!neededEnergy || neededEnergy / remainingEnergy < 0.25)
			&& (!outputHeat || ship->HeatFraction() + outputHeat < .9))
		return true;

	return false;
}



bool ShipAttributeHandler::SilentJumps() const
{
	return silentJumps;
}



double ShipAttributeHandler::CloakFuelCost() const
{
	// The fuel cost of cloaking is not only the cost of the cloak itself,
	// but also the natural fuel gain or lost due to fuel consumption and generation.
	// If fuel generation outpaces fuel lost due to cloaking or fuel consumption, then consider
	// the fuel cost to be 0.
	return min(0., cloakCost.fuel + fuelConsumption - fuelGeneration);
}



bool ShipAttributeHandler::HasFuelForCloak(const ResourceLevels &available) const
{
	double fuelCost = CloakFuelCost();
	// Don't cloak if it would result in you becoming stranded.
	// If the ship has a ramscoop, assume that it won't be stranded due to cloak usage.
	if(fuelCost && !ramscoop)
	{
		double fuel = available.fuel;
		int steps = ceil((1. - ship->Cloaking()) / ship->CloakingSpeed());
		// Only cloak if you will be able to fully cloak and also maintain it
		// for as long as it will take you to reach full cloak.
		fuel -= CloakFuelCost() * (1 + 2 * steps);
		if(fuel < ship->JumpNavigation().JumpFuel())
			return false;
	}
	return true;
}



bool ShipAttributeHandler::CanRecoverHullWhileCloaked() const
{
	if(cloakedRepairMult > -1.)
	{
		if(hullRepairRate > 0.)
			return true;
		if(cloakingHullDelay < 1. && hullRepairRateWithDelay > 0.)
			return true;
	}
	return false;
}



bool ShipAttributeHandler::CanRecoverShieldsWhileCloaked() const
{
	if(cloakedRegenMult > -1.)
	{
		if(shieldRegenRate > 0.)
			return true;
		if(cloakingShieldDelay < 1. && shieldRegenRateWithDelay > 0.)
			return true;
	}
	return false;
}



double ShipAttributeHandler::TurretTurnMultiplier() const
{
	return turretTurnMult;
}



const ResourceLevels &ShipAttributeHandler::DamageProtection() const
{
	return damageProtection;
}



double ShipAttributeHandler::PiercingProtection() const
{
	return piercingProtection;
}



double ShipAttributeHandler::PiercingResistance() const
{
	return piercingResistance;
}



double ShipAttributeHandler::HighShieldPermeability() const
{
	return highShieldPermeability;
}



double ShipAttributeHandler::LowShieldPermeability() const
{
	return lowShieldPermeability;
}



double ShipAttributeHandler::CloakedShieldPermeability() const
{
	return cloakedShieldPermeability;
}



double ShipAttributeHandler::CloakedHullProtection() const
{
	return cloakedHullProtection;
}



double ShipAttributeHandler::CloakedShieldProtection() const
{
	return cloakedShieldProtection;
}



double ShipAttributeHandler::ForceProtection() const
{
	return forceProtection;
}



void ShipAttributeHandler::Capacity()
{
	ship->capacities.hull = attributes->Get("hull") * (1 + attributes->Get("hull multiplier"));
	ship->capacities.shields = attributes->Get("shields") * (1 + attributes->Get("shield multiplier"));
	ship->capacities.energy = attributes->Get("energy capacity");
	// Heat capacity is dictated by factors other than attributes
	// and therefore isn't saved here.
	ship->capacities.fuel = attributes->Get("fuel capacity");

	// DoT counters do not have capacities.

	if(ship->neverDisabled)
		ship->minimumHull = 0.;
	else
	{
		double absoluteThreshold = attributes->Get("absolute threshold");
		if(absoluteThreshold > 0.)
			ship->minimumHull = absoluteThreshold;
		else
		{
			double thresholdPercent = attributes->Get("threshold percentage");
			double transition = 1 / (1 + 0.0005 * ship->capacities.hull);
			ship->minimumHull = ship->capacities.hull * (thresholdPercent > 0.
				? min(thresholdPercent, 1.) : 0.1 * (1. - transition) + 0.5 * transition);
			ship->minimumHull = max(0., floor(ship->minimumHull + attributes->Get("hull threshold")));
		}
	}

	outfitCapacity = ship->BaseAttributes().Get("outfit space");
	weaponCapacity = ship->BaseAttributes().Get("weapon capacity");
	engineCapacity = ship->BaseAttributes().Get("engine capacity");

	cargoSpace = attributes->Get("cargo space");
	automaton = attributes->Get("automaton");
	requiredCrew = attributes->Get("required crew");
	mandatoryCrew = attributes->Get("mandatory crew");
	bunks = attributes->Get("bunks");
	crewEquiv = attributes->Get("crew equivalent");
	onlyUseCrewEquiv = attributes->Get("use crew equivalent as crew");
}



void ShipAttributeHandler::EnergyAndFuelGeneration()
{
	energyGeneration = attributes->Get("energy generation");
	energyConsumption = attributes->Get("energy consumption");

	fuelGeneration = attributes->Get("fuel generation");
	fuelConsumption = attributes->Get("fuel consumption");
	fuelEnergy = attributes->Get("fuel energy");
	fuelHeat = attributes->Get("fuel heat");

	ramscoop = attributes->Get("ramscoop");
	solarCollection = attributes->Get("solar collection");
	solarHeat = attributes->Get("solar heat");
}



void ShipAttributeHandler::HeatAndCooling()
{
	heatGeneration = attributes->Get("heat generation");
	heatDissipation = .001 * attributes->Get("heat dissipation");
	heatCapacity = attributes->Get("heat capacity");

	cooling = attributes->Get("cooling");
	activeCooling = attributes->Get("active cooling");
	coolingEnergy = attributes->Get("cooling energy");
	// This is an S-curve where the efficiency is 100% if you have no outfits
	// that create "cooling inefficiency", and as that value increases the
	// efficiency stays high for a while, then drops off, then approaches 0.
	double x = attributes->Get("cooling inefficiency");
	coolingInefficiency = x ? 2. + 2. / (1. + exp(x / -2.)) - 4. / (1. + exp(x / -4.)) : 1.;
}



void ShipAttributeHandler::HullRepair()
{
	repairDelay = attributes->Get("repair delay");

	hullRepairRate = (attributes->Get("hull repair rate") + attributes->Get("delayed hull repair rate"))
		* (1. + attributes->Get("hull repair multiplier"));
	hullRepairCost.energy = (attributes->Get("hull energy") + attributes->Get("delayed hull energy"))
		* (1. + attributes->Get("hull energy multiplier"));
	hullRepairCost.heat = (attributes->Get("hull heat") + attributes->Get("delayed hull heat"))
		* (1. + attributes->Get("hull heat multiplier"));
	hullRepairCost.fuel = (attributes->Get("hull fuel") + attributes->Get("delayed hull fuel"))
		* (1. + attributes->Get("hull fuel multiplier"));

	hullRepairRateWithDelay = attributes->Get("hull repair rate") * (1. + attributes->Get("hull repair multiplier"));
	hullRepairWithDelayCost.energy = attributes->Get("hull energy") * (1. + attributes->Get("hull energy multiplier"));
	hullRepairWithDelayCost.heat = attributes->Get("hull heat") * (1. + attributes->Get("hull heat multiplier"));
	hullRepairWithDelayCost.fuel = attributes->Get("hull fuel") * (1. + attributes->Get("hull fuel multiplier"));
}



void ShipAttributeHandler::ShieldRegen()
{
	depletedShieldDelay = attributes->Get("depleted shield delay");
	shieldDelay = attributes->Get("shield delay");

	shieldRegenRate = (attributes->Get("shield generation") + attributes->Get("delayed shield generation"))
		* (1. + attributes->Get("shield generation multiplier"));
	shieldRegenCost.energy = (attributes->Get("shield energy") + attributes->Get("delayed shield energy"))
		* (1. + attributes->Get("shield energy multiplier"));
	shieldRegenCost.heat = (attributes->Get("shield heat") + attributes->Get("delayed shield heat"))
		* (1. + attributes->Get("shield heat multiplier"));
	shieldRegenCost.fuel = (attributes->Get("shield fuel") + attributes->Get("delayed shield fuel"))
		* (1. + attributes->Get("shield fuel multiplier"));

	shieldRegenRateWithDelay = attributes->Get("shield generation")
		* (1. + attributes->Get("shield generation multiplier"));
	shieldRegenWithDelayCost.energy = attributes->Get("shield energy")
		* (1. + attributes->Get("shield energy multiplier"));
	shieldRegenWithDelayCost.heat = attributes->Get("shield heat") * (1. + attributes->Get("shield heat multiplier"));
	shieldRegenWithDelayCost.fuel = attributes->Get("shield fuel") * (1. + attributes->Get("shield fuel multiplier"));
}



void ShipAttributeHandler::Recovery()
{
	recoveryTime = attributes->Get("disabled recovery time");

	recoveryCost.energy = attributes->Get("disabled recovery energy");
	recoveryCost.fuel = attributes->Get("disabled recovery fuel");
	recoveryCost.heat = attributes->Get("disabled recovery heat");
	recoveryCost.ionization = attributes->Get("disabled recovery ionization");
	recoveryCost.scrambling = attributes->Get("disabled recovery scrambling");
	recoveryCost.disruption = attributes->Get("disabled recovery disruption");
	recoveryCost.slowness = attributes->Get("disabled recovery slowing");
	recoveryCost.discharge = attributes->Get("disabled recovery discharge");
	recoveryCost.corrosion = attributes->Get("disabled recovery corrosion");
	recoveryCost.leakage = attributes->Get("disabled recovery leak");
	recoveryCost.burning = attributes->Get("disabled recovery burning");
}



void ShipAttributeHandler::Thrust()
{
	thrust = attributes->Get("thrust");

	thrustCost.hull = attributes->Get("thrusting hull");
	thrustCost.shields = attributes->Get("thrusting shields");
	thrustCost.energy = attributes->Get("thrusting energy");
	thrustCost.heat = attributes->Get("thrusting heat");
	thrustCost.fuel = attributes->Get("thrusting fuel");

	thrustCost.corrosion = attributes->Get("thrusting corrosion");
	thrustCost.discharge = attributes->Get("thrusting discharge");
	thrustCost.ionization = attributes->Get("thrusting ion");
	thrustCost.scrambling = attributes->Get("thrusting scramble");
	thrustCost.burning = attributes->Get("thrusting burn");
	thrustCost.leakage = attributes->Get("thrusting leakage");
	thrustCost.disruption = attributes->Get("thrusting disruption");
	thrustCost.slowness = attributes->Get("thrusting slowing");
}



void ShipAttributeHandler::Turn()
{
	turn = attributes->Get("turn");

	turnCost.hull = attributes->Get("turning hull");
	turnCost.shields = attributes->Get("turning shields");
	turnCost.energy = attributes->Get("turning energy");
	turnCost.heat = attributes->Get("turning heat");
	turnCost.fuel = attributes->Get("turning fuel");

	turnCost.corrosion = attributes->Get("turning corrosion");
	turnCost.discharge = attributes->Get("turning discharge");
	turnCost.ionization = attributes->Get("turning ion");
	turnCost.scrambling = attributes->Get("turn scramble");
	turnCost.burning = attributes->Get("turning burn");
	turnCost.leakage = attributes->Get("turning leakage");
	turnCost.disruption = attributes->Get("turning disruption");
	turnCost.slowness = attributes->Get("turning slowing");
}



void ShipAttributeHandler::ReverseThrust()
{
	reverseThrust = attributes->Get("reverse thrust");

	reverseThrustCost.hull = attributes->Get("reverse thrusting hull");
	reverseThrustCost.shields = attributes->Get("reverse thrusting shields");
	reverseThrustCost.energy = attributes->Get("reverse thrusting energy");
	reverseThrustCost.heat = attributes->Get("reverse thrusting heat");
	reverseThrustCost.fuel = attributes->Get("reverse thrusting fuel");

	reverseThrustCost.corrosion = attributes->Get("reverse thrusting corrosion");
	reverseThrustCost.discharge = attributes->Get("reverse thrusting discharge");
	reverseThrustCost.ionization = attributes->Get("reverse thrusting ion");
	reverseThrustCost.scrambling = attributes->Get("reverse thrusting scramble");
	reverseThrustCost.burning = attributes->Get("reverse thrusting burn");
	reverseThrustCost.leakage = attributes->Get("reverse thrusting leakage");
	reverseThrustCost.disruption = attributes->Get("reverse thrusting disruption");
	reverseThrustCost.slowness = attributes->Get("reverse thrusting slowing");
}



void ShipAttributeHandler::AfterburnerThrust()
{
	afterburnerThrust = attributes->Get("afterburner thrust");

	afterburnerThrustCost.hull = attributes->Get("afterburner hull");
	afterburnerThrustCost.shields = attributes->Get("afterburner shields");
	afterburnerThrustCost.energy = attributes->Get("afterburner energy");
	afterburnerThrustCost.heat = attributes->Get("afterburner heat");
	afterburnerThrustCost.fuel = attributes->Get("afterburner fuel");

	afterburnerThrustCost.corrosion = attributes->Get("afterburner corrosion");
	afterburnerThrustCost.discharge = attributes->Get("afterburner discharge");
	afterburnerThrustCost.ionization = attributes->Get("afterburner ion");
	afterburnerThrustCost.scrambling = attributes->Get("afterburner scramble");
	afterburnerThrustCost.burning = attributes->Get("afterburner burn");
	afterburnerThrustCost.leakage = attributes->Get("afterburner leakage");
	afterburnerThrustCost.disruption = attributes->Get("afterburner disruption");
	afterburnerThrustCost.slowness = attributes->Get("afterburner slowing");
}



void ShipAttributeHandler::Cloaking()
{
	cloakCost.shields = attributes->Get("cloaking shields");
	cloakCost.hull = attributes->Get("cloaking hull");
	cloakCost.energy = attributes->Get("cloaking energy");
	cloakCost.fuel = attributes->Get("cloaking fuel");
	cloakCost.heat += attributes->Get("cloaking heat");

	cloak = attributes->Get("cloak");
	cloakByMass = attributes->Get("cloak by mass");
	cloakHullThreshold = attributes->Get("cloak hull threshold");
	cloakingShieldDelay = attributes->Get("cloaking shield delay");
	cloakingHullDelay = attributes->Get("cloaking repair delay");
	cloakPhasing = attributes->Get("cloak phasing");

	// Unlike other multipliers, these attributes are not added to 1 since the multiplier is
	// only active if the ship is cloaking.
	cloakedRepairMult = attributes->Get("cloaked repair multiplier");
	cloakedRegenMult = attributes->Get("cloaked regen multiplier");

	cloakedFiring = attributes->Get("cloaked firing");
	canAfterburnerWhileCloaked = attributes->Get("cloaked afterburner");
	canBoardWhileCloaked = attributes->Get("cloaked boarding");
	canCommunicateWhileCloaked = attributes->Get("cloaked communication");
	canFireWhileCloaked = cloakedFiring;
	canPickupWhileCloaked = attributes->Get("cloaked pickup");
	canScanWhileCloaked = attributes->Get("cloaked scanning");
	canDeployWhileCloaked = attributes->Get("cloaked deployment");
}



void ShipAttributeHandler::Scanning()
{
	cargoScanPower = attributes->Get("cargo scan power");
	outfitScanPower = attributes->Get("outfit scan power");
	cargoScanSpeed = attributes->Get("cargo scan efficiency");
	outfitScanSpeed = attributes->Get("outfit scan efficiency");
	cargoScanOpacity = attributes->Get("cargo scan opacity");
	outfitScanOpacity = attributes->Get("outfit scan opacity");
	asteroidScanPower = attributes->Get("asteroid scan power");
	atmosphereScan = attributes->Get("atmosphere scan");
	silentScans = attributes->Get("silent scans");
	inscrutable = attributes->Get("inscrutable");
}



void ShipAttributeHandler::Misc()
{
	overheatDamageThreshold = 1. + attributes->Get("overheat damage threshold");
	overheatDamageRate = attributes->Get("overheat damage rate");

	drag = attributes->Get("drag");
	dragReduction = 1. + attributes->Get("drag reduction");
	accelerationMult = 1. + attributes->Get("acceleration multiplier");
	inertiaReduction = 1. + attributes->Get("inertia reduction");
	turnMult = 1. + attributes->Get("turn multiplier");

	landingSpeed = attributes->Get("landing speed");
	silentJumps = attributes->Get("silent jumps");
	selfDestruct = attributes->Get("self destruct");

	turretTurnMult = 1. + attributes->Get("turret turn multiplier");

	piercingProtection = 1. + attributes->Get("piercing protection");
	piercingResistance = attributes->Get("piercing resistance");
	highShieldPermeability = attributes->Get("high shield permeability");
	lowShieldPermeability = attributes->Get("low shield permeability");
	cloakedShieldPermeability = attributes->Get("cloaked shield permeability");
	cloakedHullProtection = attributes->Get("cloak hull protection");
	cloakedShieldProtection = attributes->Get("cloak shield protection");
	damageProtection.shields = 1. + attributes->Get("shield protection");
	damageProtection.hull = 1. + attributes->Get("hull protection");
	damageProtection.energy = 1. + attributes->Get("energy protection");
	damageProtection.fuel = 1. + attributes->Get("fuel protection");
	damageProtection.heat = 1. + attributes->Get("heat protection");
	damageProtection.discharge = 1. + attributes->Get("discharge protection");
	damageProtection.corrosion = 1. + attributes->Get("corrosion protection");
	damageProtection.ionization = 1. + attributes->Get("ion protection");
	damageProtection.burning = 1. + attributes->Get("burn protection");
	damageProtection.leakage = 1. + attributes->Get("leak protection");
	damageProtection.slowness = 1. + attributes->Get("slowing protection");
	damageProtection.scrambling = 1. + attributes->Get("scramble protection");
	damageProtection.disruption = 1. + attributes->Get("disruption protection");
	forceProtection = 1. + attributes->Get("force protection");
}
