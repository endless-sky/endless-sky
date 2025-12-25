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



// Update the stored ResourceLevels for various actions a
// ship can take (e.g. regenerating shields, thrusting).
void ShipAttributeHandler::Update(const Outfit &attributes)
{
	Capacity(attributes);

	HullRepair(attributes);
	ShieldRegen(attributes);

	CorrosionResist(attributes);
	DischargeResist(attributes);
	IonizationResist(attributes);
	ScramblingResist(attributes);
	BurnResist(attributes);
	LeakageResist(attributes);
	DisruptionResist(attributes);
	SlownessResist(attributes);

	Thrust(attributes);
	Turn(attributes);
	ReverseThrust(attributes);
	AfterburnerThrust(attributes);

	Cloak(attributes);
}



void ShipAttributeHandler::Clamp(ResourceLevels &input) const
{
	input.hull = min(input.hull, capacity.hull);
	input.shields = max(0., min(input.shields, capacity.shields));
	input.energy = max(0., input.energy);
	input.fuel = max(0., input.fuel);
	input.heat = max(0., input.heat);
}



// Clear all levels of input and set hull to -1.
void ShipAttributeHandler::Kill(ResourceLevels &input) const
{
	input.hull = -1.;
	input.shields = 0.;
	input.energy = 0.;
	input.heat = 0.;
	input.fuel = 0.;
	ClearDoT(input);
}



// Clear the damage over time levels of the input.
void ShipAttributeHandler::ClearDoT(ResourceLevels &input) const
{
	input.discharge = 0.;
	input.corrosion = 0.;
	input.scrambling = 0.;
	input.ionization = 0.;
	input.leakage = 0.;
	input.burning = 0.;
	input.disruption = 0.;
	input.slowness = 0.;
}



// Repair the given stat up to the maximum given the energy input and cost.
// Updates the available variable with the remaining amount of repairs that
// can be done.
void ShipAttributeHandler::DoRepair(double &stat, double &available, double maximum, ResourceLevels &input, const ResourceLevels &cost) const
{
	if(available <= 0. || stat >= maximum)
		return;

	if(cost.energy > 0.)
		available = min(available, input.energy / cost.energy);
	if(cost.heat < 0.)
		available = min(available, input.heat / -cost.heat);
	if(cost.fuel > 0.)
		available = min(available, input.fuel / cost.fuel);

	double transfer = min(available, maximum - stat);
	if(transfer > 0.)
	{
		stat += transfer;
		available -= transfer;
		input.energy -= transfer * cost.energy;
		input.heat += transfer * cost.heat;
		input.fuel -= transfer * cost.fuel;
	}
}



// Apply status effects and DoT resistances to the input.
void ShipAttributeHandler::DoStatusEffects(ResourceLevels &input, bool disabled) const
{
	input.hull -= input.corrosion;
	input.shields -= input.discharge;
	input.energy -= input.ionization;
	input.heat += input.burning;
	input.fuel -= input.leakage;

	// TODO: Mothership gives status resistance to carried ships?
	auto DoResistance = [&input, &disabled](double &stat, const ResourceLevels &cost)
	{
		if(!stat)
			return;

		if(disabled || cost.wildcard <= 0.)
		{
			stat = max(0., .99 * stat);
			return;
		}

		// Calculate how much resistance can be used assuming no
		// resource cost.
		double resistance = .99 * stat - max(0., .99 * stat - cost.wildcard);

		// Limit the resistance by the available resources.
		if(cost.energy > 0.)
			resistance = min(resistance, input.energy / cost.energy);
		if(cost.heat < 0.)
			resistance = min(resistance, input.heat / -cost.heat);
		if(cost.fuel > 0.)
			resistance = min(resistance, input.fuel / cost.fuel);

		if(resistance > 0.)
		{
			stat = max(0., .99 * stat - resistance);
			input.energy -= resistance * cost.energy;
			input.heat += resistance * cost.heat;
			input.fuel -= resistance * cost.fuel;
		}
		else
			stat = max(0., .99 * stat);
	};

	DoResistance(input.corrosion, corrosionResist);
	DoResistance(input.discharge, dischargeResist);
	DoResistance(input.ionization, ionizationResist);
	DoResistance(input.scrambling, scramblingResist);
	DoResistance(input.burning, burnResist);
	DoResistance(input.leakage, leakageResist);
	DoResistance(input.disruption, disruptionResist);
	DoResistance(input.slowness, slownessResist);
}



// Return true if the given input has the energy to expend on the cost. Does
// not check DoT levels.
bool ShipAttributeHandler::CanExpendBasic(const ResourceLevels &input, const ResourceLevels &cost) const
{
	if(input.hull < cost.hull)
		return false;
	if(input.shields < cost.shields)
		return false;
	if(input.energy < cost.energy)
		return false;
	if(input.heat < -cost.heat)
		return false;
	if(input.fuel < cost.fuel)
		return false;
	return true;
}



// Return true if the given input has the energy to expend on the entire cost.
bool ShipAttributeHandler::CanExpend(const ResourceLevels &input, const ResourceLevels &cost) const
{
	if(!CanExpendBasic(input, cost))
		return false;
	if(input.corrosion < -cost.corrosion)
		return false;
	if(input.discharge < -cost.discharge)
		return false;
	if(input.ionization < -cost.ionization)
		return false;
	if(input.burning < -cost.burning)
		return false;
	if(input.leakage < -cost.leakage)
		return false;
	if(input.disruption < -cost.disruption)
		return false;
	if(input.slowness < -cost.slowness)
		return false;
	return true;
}



// Return true if the given input has the energy to expend on the firing cost.
// This ignores any shield costs, allowing ships to fire a weapon even with
// no shields. This also prevents a ship from disabling itself as a result
// of any firing hull cost.
ShipAttributeHandler::CanFireResult ShipAttributeHandler::CanFire(const ResourceLevels &input, const ResourceLevels &cost) const
{
	// We do check hull, but we don't check shields. Ships can survive with all shields depleted.
	// Ships should not disable themselves, so we check if we stay above minimumHull.
	if(input.hull - capacity.wildcard < cost.hull)
		return CanFireResult::NO_HULL;
	if(input.energy < cost.energy)
		return CanFireResult::NO_ENERGY;
	// If a weapon requires heat to fire, (rather than generating heat), we must
	// have enough heat to spare.
	if(input.heat < -cost.heat)
		return CanFireResult::NO_HEAT;
	// Repeat this for various effects which shouldn't drop below 0.
	if(input.fuel < cost.fuel)
		return CanFireResult::NO_FUEL;
	if(input.corrosion < -cost.corrosion)
		return CanFireResult::NO_CORROSION;
	if(input.discharge < -cost.discharge)
		return CanFireResult::NO_DISCHARGE;
	if(input.ionization < -cost.ionization)
		return CanFireResult::NO_ION;
	if(input.burning < -cost.burning)
		return CanFireResult::NO_BURNING;
	if(input.leakage < -cost.leakage)
		return CanFireResult::NO_LEAKAGE;
	if(input.disruption < -cost.disruption)
		return CanFireResult::NO_DISRUPTION;
	if(input.slowness < -cost.slowness)
		return CanFireResult::NO_SLOWING;
	return CanFireResult::CAN_FIRE;
}



// Return the fraction of 100% output that the input can manage given the cost.
double ShipAttributeHandler::FractionalUsage(const ResourceLevels &input, const ResourceLevels &cost) const
{
	double scale = 1.;
	auto ScaleOutput = [&scale](double input, double cost)
	{
		if(input < cost * scale)
			scale = input / cost;
	};
	ScaleOutput(input.hull, cost.hull);
	ScaleOutput(input.shields, cost.shields);
	ScaleOutput(input.energy, cost.energy);
	ScaleOutput(input.heat, -cost.heat);
	ScaleOutput(input.fuel, cost.fuel);
	ScaleOutput(input.corrosion, -cost.corrosion);
	ScaleOutput(input.discharge, -cost.discharge);
	ScaleOutput(input.ionization, -cost.ionization);
	ScaleOutput(input.burning, -cost.burning);
	ScaleOutput(input.leakage, -cost.leakage);
	ScaleOutput(input.disruption, -cost.disruption);
	ScaleOutput(input.slowness, -cost.slowness);

	return scale;
}



// Apply damage * scale to the input. Hull, shields, energy, and fuel
// are subtracted from input while all other levels are added to input.
// Does not apply damage to DoT levels.
void ShipAttributeHandler::DamageBasic(ResourceLevels &input, const ResourceLevels &damage, double scale) const
{
	input.hull -= scale * damage.hull;
	input.shields -= scale * damage.shields;
	input.energy -= scale * damage.energy;
	input.heat += scale * damage.heat;
	input.fuel -= scale * damage.fuel;
}



// Applies damage to all levels.
void ShipAttributeHandler::Damage(ResourceLevels &input, const ResourceLevels &damage, double scale) const
{
	DamageBasic(input, damage, scale);

	input.corrosion += scale * damage.corrosion;
	input.discharge += scale * damage.discharge;
	input.ionization += scale * damage.ionization;
	input.scrambling = scale * damage.scrambling;
	input.burning += scale * damage.burning;
	input.leakage += scale * damage.leakage;
	input.disruption += scale * damage.disruption;
	input.slowness += scale * damage.slowness;
}



// Construct an ResourceLevels object for the firing cost of the given weapon
// when fired from the given ship.
ResourceLevels ShipAttributeHandler::FiringCost(const Weapon &weapon, const Ship &ship) const
{
	ResourceLevels cost;
	double maxHeat = weapon.RelativeFiringHeat() ? ship.MaximumHeat() : 0.;

	cost.hull = weapon.FiringHull() + weapon.RelativeFiringHull() * capacity.hull;
	cost.shields = weapon.FiringShields() + weapon.RelativeFiringShields() * capacity.shields;
	cost.energy = weapon.FiringEnergy() + weapon.RelativeFiringEnergy() * capacity.energy;
	cost.heat = weapon.FiringHeat() + weapon.RelativeFiringHeat() * maxHeat;
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
	cost.shields = min(cost.shields, ship.ShieldLevel());

	return cost;
}



// Update the stored capacity for various ResourceLevels on a ship.
void ShipAttributeHandler::Capacity(const Outfit &attributes)
{
	capacity.hull = attributes.Get("hull") * (1 + attributes.Get("hull multiplier"));
	capacity.shields = attributes.Get("shields") * (1 + attributes.Get("shield multiplier"));
	capacity.energy = attributes.Get("energy capacity");
	// Heat capacity is dictated by factors other than attributes
	// and therefore isn't saved here.
	capacity.fuel = attributes.Get("fuel capacity");

	// DoT counters do not have capacities.

	// Cache a ship's minimum hull in the capacity wildcard.
	double absoluteThreshold = attributes.Get("absolute threshold");
	if(absoluteThreshold > 0.)
		capacity.wildcard = absoluteThreshold;
	else
	{
		double thresholdPercent = attributes.Get("threshold percentage");
		double transition = 1 / (1 + 0.0005 * capacity.hull);
		double minimumHull = capacity.hull * (thresholdPercent > 0. ? min(thresholdPercent, 1.) : 0.1 * (1. - transition) + 0.5 * transition);

		capacity.wildcard = max(0., floor(minimumHull + attributes.Get("hull threshold")));
	}
}



// Update the stored ResourceLevels for each action a ship can take.
void ShipAttributeHandler::HullRepair(const Outfit &attributes)
{
	hullRepair.wildcard = attributes.Get("hull repair rate") * (1. + attributes.Get("hull repair multiplier"));
	hullRepair.energy = attributes.Get("hull energy") * (1. + attributes.Get("hull energy multiplier"));
	hullRepair.heat = attributes.Get("hull heat") * (1. + attributes.Get("hull heat multiplier"));
	hullRepair.fuel = attributes.Get("hull fuel") * (1. + attributes.Get("hull fuel multiplier"));

	hullRepairNoDelay.wildcard = (attributes.Get("hull repair rate") + attributes.Get("delayed hull repair rate"))
		* (1. + attributes.Get("hull repair multiplier"));
	hullRepairNoDelay.energy = (attributes.Get("hull energy") + attributes.Get("delayed hull energy"))
		* (1. + attributes.Get("hull energy multiplier"));
	hullRepairNoDelay.heat = (attributes.Get("hull heat") + attributes.Get("delayed hull heat"))
		* (1. + attributes.Get("hull heat multiplier"));
	hullRepairNoDelay.fuel = (attributes.Get("hull fuel") + attributes.Get("delayed hull fuel"))
		* (1. + attributes.Get("hull fuel multiplier"));
}



void ShipAttributeHandler::ShieldRegen(const Outfit &attributes)
{
	shieldRegen.wildcard = attributes.Get("shield generation") * (1. + attributes.Get("shield generation multiplier"));
	shieldRegen.energy = attributes.Get("shield energy") * (1. + attributes.Get("shield energy multiplier"));
	shieldRegen.heat = attributes.Get("shield heat") * (1. + attributes.Get("shield heat multiplier"));
	shieldRegen.fuel = attributes.Get("shield fuel") * (1. + attributes.Get("shield fuel multiplier"));

	shieldRegenNoDelay.wildcard = (attributes.Get("shield generation") + attributes.Get("delayed shield generation"))
		* (1. + attributes.Get("shield generation multiplier"));
	shieldRegenNoDelay.energy = (attributes.Get("shield energy") + attributes.Get("delayed shield energy"))
		* (1. + attributes.Get("shield energy multiplier"));
	shieldRegenNoDelay.heat = (attributes.Get("shield heat") + attributes.Get("delayed shield heat"))
		* (1. + attributes.Get("shield heat multiplier"));
	shieldRegenNoDelay.fuel = (attributes.Get("shield fuel") + attributes.Get("delayed shield fuel"))
		* (1. + attributes.Get("shield fuel multiplier"));
}



void ShipAttributeHandler::CorrosionResist(const Outfit &attributes)
{
	corrosionResist.wildcard = attributes.Get("corrosion resistance");
	// Save resistance costs as per unit of resistance.
	corrosionResist.energy = attributes.Get("corrosion resistance energy") / corrosionResist.wildcard;
	corrosionResist.heat = attributes.Get("corrosion resistance heat") / corrosionResist.wildcard;
	corrosionResist.fuel = attributes.Get("corrosion resistance fuel") / corrosionResist.wildcard;
}



void ShipAttributeHandler::DischargeResist(const Outfit &attributes)
{
	dischargeResist.wildcard = attributes.Get("discharge resistance");
	// Save resistance costs as per unit of resistance.
	dischargeResist.energy = attributes.Get("discharge resistance energy") / dischargeResist.wildcard;
	dischargeResist.heat = attributes.Get("discharge resistance heat") / dischargeResist.wildcard;
	dischargeResist.fuel = attributes.Get("discharge resistance fuel") / dischargeResist.wildcard;
}



void ShipAttributeHandler::IonizationResist(const Outfit &attributes)
{
	ionizationResist.wildcard = attributes.Get("ion resistance");
	// Save resistance costs as per unit of resistance.
	ionizationResist.energy = attributes.Get("ion resistance energy") / ionizationResist.wildcard;
	ionizationResist.heat = attributes.Get("ion resistance heat") / ionizationResist.wildcard;
	ionizationResist.fuel = attributes.Get("ion resistance fuel") / ionizationResist.wildcard;
}



void ShipAttributeHandler::ScramblingResist(const Outfit &attributes)
{
	scramblingResist.wildcard = attributes.Get("scramble resistance");
	// Save resistance costs as per unit of resistance.
	scramblingResist.energy = attributes.Get("scramble resistance energy") / scramblingResist.wildcard;
	scramblingResist.heat = attributes.Get("scramble resistance heat") / scramblingResist.wildcard;
	scramblingResist.fuel = attributes.Get("scramble resistance fuel") / scramblingResist.wildcard;
}



void ShipAttributeHandler::BurnResist(const Outfit &attributes)
{
	burnResist.wildcard = attributes.Get("burn resistance");
	// Save resistance costs as per unit of resistance.
	burnResist.energy = attributes.Get("burn resistance energy") / burnResist.wildcard;
	burnResist.heat = attributes.Get("burn resistance heat") / burnResist.wildcard;
	burnResist.fuel = attributes.Get("burn resistance fuel") / burnResist.wildcard;
}



void ShipAttributeHandler::LeakageResist(const Outfit &attributes)
{
	leakageResist.wildcard = attributes.Get("leak resistance");
	// Save resistance costs as per unit of resistance.
	leakageResist.energy = attributes.Get("leak resistance energy") / leakageResist.wildcard;
	leakageResist.heat = attributes.Get("leak resistance heat") / leakageResist.wildcard;
	leakageResist.fuel = attributes.Get("leak resistance fuel") / leakageResist.wildcard;
}



void ShipAttributeHandler::DisruptionResist(const Outfit &attributes)
{
	disruptionResist.wildcard = attributes.Get("disruption resistance");
	// Save resistance costs as per unit of resistance.
	disruptionResist.energy = attributes.Get("disruption resistance energy") / disruptionResist.wildcard;
	disruptionResist.heat = attributes.Get("disruption resistance heat") / disruptionResist.wildcard;
	disruptionResist.fuel = attributes.Get("disruption resistance fuel") / disruptionResist.wildcard;
}



void ShipAttributeHandler::SlownessResist(const Outfit &attributes)
{
	slownessResist.wildcard = attributes.Get("slowing resistance");
	// Save resistance costs as per unit of resistance.
	slownessResist.energy = attributes.Get("slowing resistance energy") / slownessResist.wildcard;
	slownessResist.heat = attributes.Get("slowing resistance heat") / slownessResist.wildcard;
	slownessResist.fuel = attributes.Get("slowing resistance fuel") / slownessResist.wildcard;
}



void ShipAttributeHandler::Thrust(const Outfit &attributes)
{
	thrust.wildcard = attributes.Get("thrust");

	thrust.hull = attributes.Get("thrusting hull");
	thrust.shields = attributes.Get("thrusting shields");
	thrust.energy = attributes.Get("thrusting energy");
	thrust.heat = attributes.Get("thrusting heat");
	thrust.fuel = attributes.Get("thrusting fuel");

	thrust.corrosion = attributes.Get("thrusting corrosion");
	thrust.discharge = attributes.Get("thrusting discharge");
	thrust.ionization = attributes.Get("thrusting ion");
	thrust.scrambling = attributes.Get("thrusting scramble");
	thrust.burning = attributes.Get("thrusting burn");
	thrust.leakage = attributes.Get("thrusting leakage");
	thrust.disruption = attributes.Get("thrusting disruption");
	thrust.slowness = attributes.Get("thrusting slowing");
}



void ShipAttributeHandler::Turn(const Outfit &attributes)
{
	turn.wildcard = attributes.Get("turn");

	turn.hull = attributes.Get("turning hull");
	turn.shields = attributes.Get("turning shields");
	turn.energy = attributes.Get("turning energy");
	turn.heat = attributes.Get("turning heat");
	turn.fuel = attributes.Get("turning fuel");

	turn.corrosion = attributes.Get("turning corrosion");
	turn.discharge = attributes.Get("turning discharge");
	turn.ionization = attributes.Get("turning ion");
	turn.scrambling = attributes.Get("turn scramble");
	turn.burning = attributes.Get("turning burn");
	turn.leakage = attributes.Get("turning leakage");
	turn.disruption = attributes.Get("turning disruption");
	turn.slowness = attributes.Get("turning slowing");
}



void ShipAttributeHandler::ReverseThrust(const Outfit &attributes)
{
	reverseThrust.wildcard = attributes.Get("reverse thrust");

	reverseThrust.hull = attributes.Get("reverse thrusting hull");
	reverseThrust.shields = attributes.Get("reverse thrusting shields");
	reverseThrust.energy = attributes.Get("reverse thrusting energy");
	reverseThrust.heat = attributes.Get("reverse thrusting heat");
	reverseThrust.fuel = attributes.Get("reverse thrusting fuel");

	reverseThrust.corrosion = attributes.Get("reverse thrusting corrosion");
	reverseThrust.discharge = attributes.Get("reverse thrusting discharge");
	reverseThrust.ionization = attributes.Get("reverse thrusting ion");
	reverseThrust.scrambling = attributes.Get("reverse thrusting scramble");
	reverseThrust.burning = attributes.Get("reverse thrusting burn");
	reverseThrust.leakage = attributes.Get("reverse thrusting leakage");
	reverseThrust.disruption = attributes.Get("reverse thrusting disruption");
	reverseThrust.slowness = attributes.Get("reverse thrusting slowing");
}



void ShipAttributeHandler::AfterburnerThrust(const Outfit &attributes)
{
	afterburnerThrust.wildcard = attributes.Get("afterburner thrust");

	afterburnerThrust.hull = attributes.Get("afterburner hull");
	afterburnerThrust.shields = attributes.Get("afterburner shields");
	afterburnerThrust.energy = attributes.Get("afterburner energy");
	afterburnerThrust.heat = attributes.Get("afterburner heat");
	afterburnerThrust.fuel = attributes.Get("afterburner fuel");

	afterburnerThrust.corrosion = attributes.Get("afterburner corrosion");
	afterburnerThrust.discharge = attributes.Get("afterburner discharge");
	afterburnerThrust.ionization = attributes.Get("afterburner ion");
	afterburnerThrust.scrambling = attributes.Get("afterburner scramble");
	afterburnerThrust.burning = attributes.Get("afterburner burn");
	afterburnerThrust.leakage = attributes.Get("afterburner leakage");
	afterburnerThrust.disruption = attributes.Get("afterburner disruption");
	afterburnerThrust.slowness = attributes.Get("afterburner slowing");
}



void ShipAttributeHandler::Cloak(const Outfit &attributes)
{
	cloak.wildcard = attributes.Get("cloak");

	cloak.energy = attributes.Get("cloaking energy");
	cloak.heat = attributes.Get("cloaking heat");
	cloak.fuel = attributes.Get("cloaking fuel");
}