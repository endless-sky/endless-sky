/* ShipAttributeHandler.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipAttributeHandler.h"

#include "../Outfit.h"
#include "../Ship.h"
#include "../Weapon.h"

#include <cmath>

using namespace std;



void ShipAttributeHandler::Setup(const Ship *parent, ResourceLevels *levels)
{
	ship = parent;
	attributes = &parent->Attributes();
	shipLevels = levels;
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

	// DoT resistances:
	CorrosionResist();
	DischargeResist();
	IonizationResist();
	ScramblingResist();
	BurnResist();
	LeakageResist();
	DisruptionResist();
	SlownessResist();

	// Movement:
	Thrust();
	Turn();
	ReverseThrust();
	AfterburnerThrust();

	// Miscellaneous actions and attributes:
	Cloaking();
	Scanning();
	Misc();
}



void ShipAttributeHandler::Kill() const
{
	shipLevels->hull = -1.;
	shipLevels->shields = 0.;
	shipLevels->energy = 0.;
	shipLevels->heat = 0.;
	shipLevels->fuel = 0.;
	ClearDoT();
}



void ShipAttributeHandler::ClearDoT() const
{
	shipLevels->discharge = 0.;
	shipLevels->corrosion = 0.;
	shipLevels->scrambling = 0.;
	shipLevels->ionization = 0.;
	shipLevels->leakage = 0.;
	shipLevels->burning = 0.;
	shipLevels->disruption = 0.;
	shipLevels->slowness = 0.;
}



void ShipAttributeHandler::DoRepair(double &stat, double &available, double maximum, const ResourceLevels &cost) const
{
	if(available <= 0. || stat >= maximum)
		return;

	if(cost.energy > 0.)
		available = min(available, shipLevels->energy / cost.energy);
	if(cost.heat < 0.)
		available = min(available, shipLevels->heat / -cost.heat);
	if(cost.fuel > 0.)
		available = min(available, shipLevels->fuel / cost.fuel);

	double transfer = min(available, maximum - stat);
	if(transfer > 0.)
	{
		stat += transfer;
		available -= transfer;
		shipLevels->energy -= transfer * cost.energy;
		shipLevels->heat += transfer * cost.heat;
		shipLevels->fuel -= transfer * cost.fuel;
	}
}



void ShipAttributeHandler::DoStatusEffects(bool disabled) const
{
	shipLevels->hull -= shipLevels->corrosion;
	shipLevels->shields -= shipLevels->discharge;
	shipLevels->energy -= shipLevels->ionization;
	shipLevels->heat += shipLevels->burning;
	shipLevels->fuel -= shipLevels->leakage;

	// TODO: Mothership gives status resistance to carried ships?
	auto DoResistance = [this, &disabled](double &stat, double resistance, const ResourceLevels &cost)
	{
		if(!stat)
			return;

		if(disabled || resistance <= 0.)
		{
			stat = max(0., .99 * stat);
			return;
		}

		// Calculate how much resistance can be used assuming no
		// resource cost.
		resistance = .99 * stat - max(0., .99 * stat - resistance);

		// Limit the resistance by the available resources.
		if(cost.energy > 0.)
			resistance = min(resistance, shipLevels->energy / cost.energy);
		if(cost.heat < 0.)
			resistance = min(resistance, shipLevels->heat / -cost.heat);
		if(cost.fuel > 0.)
			resistance = min(resistance, shipLevels->fuel / cost.fuel);

		if(resistance > 0.)
		{
			stat = max(0., .99 * stat - resistance);
			shipLevels->energy -= resistance * cost.energy;
			shipLevels->heat += resistance * cost.heat;
			shipLevels->fuel -= resistance * cost.fuel;
		}
		else
			stat = max(0., .99 * stat);
	};

	DoResistance(shipLevels->corrosion, corrosionResistance, corrosionResistCost);
	DoResistance(shipLevels->discharge, dischargeResistance, dischargeResistCost);
	DoResistance(shipLevels->ionization, ionizationResistance, ionizationResistCost);
	DoResistance(shipLevels->scrambling, scramblingResistance, scramblingResistCost);
	DoResistance(shipLevels->burning, burnResistance, burnResistCost);
	DoResistance(shipLevels->leakage, leakResistance, leakageResistCost);
	DoResistance(shipLevels->disruption, disruptionResistance, disruptionResistCost);
	DoResistance(shipLevels->slowness, slowingResistance, slownessResistCost);
}



bool ShipAttributeHandler::CanExpend(const ResourceLevels &cost) const
{
	if(shipLevels->hull < cost.hull)
		return false;
	if(shipLevels->shields < cost.shields)
		return false;
	if(shipLevels->energy < cost.energy)
		return false;
	if(shipLevels->heat < -cost.heat)
		return false;
	if(shipLevels->fuel < cost.fuel)
		return false;
	if(shipLevels->corrosion < -cost.corrosion)
		return false;
	if(shipLevels->discharge < -cost.discharge)
		return false;
	if(shipLevels->ionization < -cost.ionization)
		return false;
	if(shipLevels->burning < -cost.burning)
		return false;
	if(shipLevels->leakage < -cost.leakage)
		return false;
	if(shipLevels->disruption < -cost.disruption)
		return false;
	if(shipLevels->slowness < -cost.slowness)
		return false;
	return true;
}



double ShipAttributeHandler::FractionalUsage(const ResourceLevels &cost) const
{
	double scale = 1.;
	auto ScaleOutput = [&scale](double input, double cost)
	{
		if(input < cost * scale)
			scale = input / cost;
	};
	ScaleOutput(shipLevels->hull, cost.hull);
	ScaleOutput(shipLevels->shields, cost.shields);
	ScaleOutput(shipLevels->energy, cost.energy);
	ScaleOutput(shipLevels->heat, -cost.heat);
	ScaleOutput(shipLevels->fuel, cost.fuel);
	ScaleOutput(shipLevels->corrosion, -cost.corrosion);
	ScaleOutput(shipLevels->discharge, -cost.discharge);
	ScaleOutput(shipLevels->ionization, -cost.ionization);
	ScaleOutput(shipLevels->burning, -cost.burning);
	ScaleOutput(shipLevels->leakage, -cost.leakage);
	ScaleOutput(shipLevels->disruption, -cost.disruption);
	ScaleOutput(shipLevels->slowness, -cost.slowness);

	return scale;
}



ResourceLevels ShipAttributeHandler::FiringCost(const Weapon &weapon) const
{
	ResourceLevels cost;

	cost.hull = weapon.FiringHull() + weapon.RelativeFiringHull() * capacity.hull;
	cost.shields = weapon.FiringShields() + weapon.RelativeFiringShields() * capacity.shields;
	cost.energy = weapon.FiringEnergy() + weapon.RelativeFiringEnergy() * capacity.energy;
	cost.heat = weapon.FiringHeat() + weapon.RelativeFiringHeat() * ship->MaximumHeat();
	cost.fuel = weapon.FiringFuel() + weapon.RelativeFiringFuel() * capacity.fuel;

	cost.corrosion = weapon.FiringCorrosion();
	cost.discharge = weapon.FiringDischarge();
	cost.ionization = weapon.FiringIon();
	cost.scrambling = weapon.FiringScramble();
	cost.burning = weapon.FiringBurn();
	cost.leakage = weapon.FiringLeak();
	cost.disruption = weapon.FiringDisruption();
	cost.slowness = weapon.FiringSlowing();

	// Ships aren't allowed to have negative shields, so clamp the firing shield
	// cost to the ship's shield level.
	cost.shields = min(cost.shields, shipLevels->shields);

	return cost;
}



ShipAttributeHandler::CanFireResult ShipAttributeHandler::CanFire(const Weapon &weapon) const
{
	ResourceLevels cost = FiringCost(weapon);
	// We do check hull, but we don't check shields. Ships can survive with all shields depleted.
	// Ships should not disable themselves, so we check if we stay above minimumHull.
	if(shipLevels->hull - minimumHull < cost.hull)
		return CanFireResult::NO_HULL;
	if(shipLevels->energy < cost.energy)
		return CanFireResult::NO_ENERGY;
	// If a weapon requires heat to fire, (rather than generating heat), we must
	// have enough heat to spare.
	if(shipLevels->heat < -cost.heat)
		return CanFireResult::NO_HEAT;
	// Repeat this for various effects which shouldn't drop below 0.
	if(shipLevels->fuel < cost.fuel)
		return CanFireResult::NO_FUEL;
	if(shipLevels->corrosion < -cost.corrosion)
		return CanFireResult::NO_CORROSION;
	if(shipLevels->discharge < -cost.discharge)
		return CanFireResult::NO_DISCHARGE;
	if(shipLevels->ionization < -cost.ionization)
		return CanFireResult::NO_ION;
	if(shipLevels->burning < -cost.burning)
		return CanFireResult::NO_BURNING;
	if(shipLevels->leakage < -cost.leakage)
		return CanFireResult::NO_LEAKAGE;
	if(shipLevels->disruption < -cost.disruption)
		return CanFireResult::NO_DISRUPTION;
	if(shipLevels->slowness < -cost.slowness)
		return CanFireResult::NO_SLOWING;
	return CanFireResult::CAN_FIRE;
}



void ShipAttributeHandler::Damage(const ResourceLevels &damage, double scale) const
{
	shipLevels->hull -= scale * damage.hull;
	shipLevels->shields -= scale * damage.shields;
	shipLevels->energy -= scale * damage.energy;
	shipLevels->heat += scale * damage.heat;
	shipLevels->fuel -= scale * damage.fuel;

	shipLevels->corrosion += scale * damage.corrosion;
	shipLevels->discharge += scale * damage.discharge;
	shipLevels->ionization += scale * damage.ionization;
	shipLevels->scrambling = scale * damage.scrambling;
	shipLevels->burning += scale * damage.burning;
	shipLevels->leakage += scale * damage.leakage;
	shipLevels->disruption += scale * damage.disruption;
	shipLevels->slowness += scale * damage.slowness;
}



double ShipAttributeHandler::FuelCapacity() const
{
	return capacity.fuel;
}



double ShipAttributeHandler::EnergyCapacity() const
{
	return capacity.energy;
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



bool ShipAttributeHandler::ShouldUseAfterburner() const
{
	double remainingFuel = shipLevels->fuel;
	double neededFuel = afterburnerThrustCost.fuel;
	double remainingEnergy = shipLevels->energy;
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
			&& (!outputHeat || ship->Heat() + outputHeat < .9))
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



bool ShipAttributeHandler::HasFuelForCloak() const
{
	double fuelCost = CloakFuelCost();
	// Don't cloak if it would result in you becoming stranded.
	// If the ship has a ramscoop, assume that it won't be stranded due to cloak usage.
	if(fuelCost && !ramscoop)
	{
		double fuel = shipLevels->fuel;
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



double ShipAttributeHandler::OpticalJamming() const
{
	return opticalJamming;
}



double ShipAttributeHandler::RadarJamming() const
{
	return radarJamming;
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
	capacity.hull = attributes->Get("hull") * (1 + attributes->Get("hull multiplier"));
	capacity.shields = attributes->Get("shields") * (1 + attributes->Get("shield multiplier"));
	capacity.energy = attributes->Get("energy capacity");
	// Heat capacity is dictated by factors other than attributes
	// and therefore isn't saved here.
	capacity.fuel = attributes->Get("fuel capacity");

	// DoT counters do not have capacities.

	double absoluteThreshold = attributes->Get("absolute threshold");
	if(absoluteThreshold > 0.)
		minimumHull = absoluteThreshold;
	else
	{
		double thresholdPercent = attributes->Get("threshold percentage");
		double transition = 1 / (1 + 0.0005 * capacity.hull);
		minimumHull = capacity.hull * (thresholdPercent > 0.
			? min(thresholdPercent, 1.) : 0.1 * (1. - transition) + 0.5 * transition);
		minimumHull = max(0., floor(minimumHull + attributes->Get("hull threshold")));
	}

	outfitCapacity = ship->BaseAttributes().Get("outfit space");
	weaponCapacity = ship->BaseAttributes().Get("weapon capacity");
	engineCapacity = ship->BaseAttributes().Get("engine capacity");

	cargoSpace = attributes->Get("cargo space");
	automaton = attributes->Get("automaton");
	requiredCrew = attributes->Get("required crew");
	bunks = attributes->Get("bunks");
	crewEquiv = attributes->Get("crew equivalent");
	onlyUseCreqEquiv = attributes->Get("use crew equivalent as crew");
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



void ShipAttributeHandler::CorrosionResist()
{
	corrosionResistance = attributes->Get("corrosion resistance");
	// Save resistance costs as per unit of resistance.
	corrosionResistCost.energy = attributes->Get("corrosion resistance energy") / corrosionResistance;
	corrosionResistCost.heat = attributes->Get("corrosion resistance heat") / corrosionResistance;
	corrosionResistCost.fuel = attributes->Get("corrosion resistance fuel") / corrosionResistance;
}



void ShipAttributeHandler::DischargeResist()
{
	dischargeResistance = attributes->Get("discharge resistance");
	// Save resistance costs as per unit of resistance.
	dischargeResistCost.energy = attributes->Get("discharge resistance energy") / dischargeResistance;
	dischargeResistCost.heat = attributes->Get("discharge resistance heat") / dischargeResistance;
	dischargeResistCost.fuel = attributes->Get("discharge resistance fuel") / dischargeResistance;
}



void ShipAttributeHandler::IonizationResist()
{
	ionizationResistance = attributes->Get("ion resistance");
	// Save resistance costs as per unit of resistance.
	ionizationResistCost.energy = attributes->Get("ion resistance energy") / dischargeResistance;
	ionizationResistCost.heat = attributes->Get("ion resistance heat") / dischargeResistance;
	ionizationResistCost.fuel = attributes->Get("ion resistance fuel") / dischargeResistance;
}



void ShipAttributeHandler::ScramblingResist()
{
	scramblingResistance = attributes->Get("scramble resistance");
	// Save resistance costs as per unit of resistance.
	scramblingResistCost.energy = attributes->Get("scramble resistance energy") / scramblingResistance;
	scramblingResistCost.heat = attributes->Get("scramble resistance heat") / scramblingResistance;
	scramblingResistCost.fuel = attributes->Get("scramble resistance fuel") / scramblingResistance;
}



void ShipAttributeHandler::BurnResist()
{
	burnResistance = attributes->Get("burn resistance");
	// Save resistance costs as per unit of resistance.
	burnResistCost.energy = attributes->Get("burn resistance energy") / burnResistance;
	burnResistCost.heat = attributes->Get("burn resistance heat") / burnResistance;
	burnResistCost.fuel = attributes->Get("burn resistance fuel") / burnResistance;
}



void ShipAttributeHandler::LeakageResist()
{
	leakResistance = attributes->Get("leak resistance");
	// Save resistance costs as per unit of resistance.
	leakageResistCost.energy = attributes->Get("leak resistance energy") / leakResistance;
	leakageResistCost.heat = attributes->Get("leak resistance heat") / leakResistance;
	leakageResistCost.fuel = attributes->Get("leak resistance fuel") / leakResistance;
}



void ShipAttributeHandler::DisruptionResist()
{
	disruptionResistance = attributes->Get("disruption resistance");
	// Save resistance costs as per unit of resistance.
	disruptionResistCost.energy = attributes->Get("disruption resistance energy") / disruptionResistance;
	disruptionResistCost.heat = attributes->Get("disruption resistance heat") / disruptionResistance;
	disruptionResistCost.fuel = attributes->Get("disruption resistance fuel") / disruptionResistance;
}



void ShipAttributeHandler::SlownessResist()
{
	slowingResistance = attributes->Get("slowing resistance");
	// Save resistance costs as per unit of resistance.
	slownessResistCost.energy = attributes->Get("slowing resistance energy") / slowingResistance;
	slownessResistCost.heat = attributes->Get("slowing resistance heat") / slowingResistance;
	slownessResistCost.fuel = attributes->Get("slowing resistance fuel") / slowingResistance;
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

	opticalJamming = attributes->Get("optical jamming");
	radarJamming = attributes->Get("radar jamming");

	turretTurnMult = 1. + attributes->Get("turret turn multiplier");

	piercingProtection = attributes->Get("piercing protection");
	piercingResistance = attributes->Get("piercing resistance");
	highShieldPermeability = attributes->Get("high shield permeability");
	lowShieldPermeability = attributes->Get("low shield permeability");
	cloakedShieldPermeability = attributes->Get("cloaked shield permeability");
	cloakedHullProtection = attributes->Get("cloak hull protection");
	cloakedShieldProtection = attributes->Get("cloak shield protection");
	damageProtection.shields = attributes->Get("shield protection");
	damageProtection.hull = attributes->Get("hull protection");
	damageProtection.energy = attributes->Get("energy protection");
	damageProtection.fuel = attributes->Get("fuel protection");
	damageProtection.heat = attributes->Get("heat protection");
	damageProtection.discharge = attributes->Get("discharge protection");
	damageProtection.corrosion = attributes->Get("corrosion protection");
	damageProtection.ionization = attributes->Get("ion protection");
	damageProtection.burning = attributes->Get("burn protection");
	damageProtection.leakage = attributes->Get("leak protection");
	damageProtection.slowness = attributes->Get("slowing protection");
	damageProtection.scrambling = attributes->Get("scramble protection");
	damageProtection.disruption = attributes->Get("disruption protection");
	forceProtection = attributes->Get("force protection");
}
