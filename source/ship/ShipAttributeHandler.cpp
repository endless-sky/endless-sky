/* ShipResourceHandler.cpp
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
	Capacity();

	HullRepair();
	ShieldRegen();

	CorrosionResist();
	DischargeResist();
	IonizationResist();
	ScramblingResist();
	BurnResist();
	LeakageResist();
	DisruptionResist();
	SlownessResist();

	Thrust();
	Turn();
	ReverseThrust();
	AfterburnerThrust();
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
		minimumHull = capacity.hull * (thresholdPercent > 0. ? min(thresholdPercent, 1.) : 0.1 * (1. - transition) + 0.5 * transition);
		minimumHull = max(0., floor(minimumHull + attributes->Get("hull threshold")));
	}
}



void ShipAttributeHandler::HullRepair()
{
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
	shieldRegenRate = (attributes->Get("shield generation") + attributes->Get("delayed shield generation"))
		* (1. + attributes->Get("shield generation multiplier"));
	shieldRegenCost.energy = (attributes->Get("shield energy") + attributes->Get("delayed shield energy"))
		* (1. + attributes->Get("shield energy multiplier"));
	shieldRegenCost.heat = (attributes->Get("shield heat") + attributes->Get("delayed shield heat"))
		* (1. + attributes->Get("shield heat multiplier"));
	shieldRegenCost.fuel = (attributes->Get("shield fuel") + attributes->Get("delayed shield fuel"))
		* (1. + attributes->Get("shield fuel multiplier"));

	shieldRegenRateWithDelay = attributes->Get("shield generation") * (1. + attributes->Get("shield generation multiplier"));
	shieldRegenWithDelayCost.energy = attributes->Get("shield energy") * (1. + attributes->Get("shield energy multiplier"));
	shieldRegenWithDelayCost.heat = attributes->Get("shield heat") * (1. + attributes->Get("shield heat multiplier"));
	shieldRegenWithDelayCost.fuel = attributes->Get("shield fuel") * (1. + attributes->Get("shield fuel multiplier"));
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
