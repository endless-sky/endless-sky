/* EnergyHandler.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "EnergyHandler.h"

#include "Outfit.h"

#include <cmath>

using namespace std;



// Update the stored EnergyLevels for various actions a
// ship can take (e.g. regenerating shields, thrusting).
void EnergyHandler::Update(const Outfit &attributes)
{
	ShieldRegen(attributes);
	HullRepair(attributes);

	CorrosionResist(attributes);
	DischargeResist(attributes);
	IonizationResist(attributes);
	BurnResist(attributes);
	LeakageResist(attributes);
	DisruptionResist(attributes);
	SlownessResist(attributes);

	Thrust(attributes);
	ReverseThrust(attributes);
	Turn(attributes);
	Afterburner(attributes);
}



// Repair the given stat up to the maximum given the energy input and cost.
// Updates the available variable with the remaining amount of repairs that
// can be done.
void EnergyHandler::DoRepair(double &stat, double &available, double maximum, EnergyLevels &input, const EnergyLevels &cost) const
{
	if(available <= 0. || stat >= maximum)
		return;

	if(cost.energy > 0.)
		available = min(available, input.energy / cost.energy);
	if(cost.fuel > 0.)
		available = min(available, input.fuel / cost.fuel);
	if(cost.heat < 0.)
		available = min(available, input.heat / -cost.heat);

	double transfer = min(available, maximum - stat);
	if(transfer > 0.)
	{
		stat += transfer;
		available -= transfer;
		input.energy -= transfer * cost.energy;
		input.fuel -= transfer * cost.fuel;
		input.heat += transfer * cost.heat;
	}
}



// Apply status effects and DoT resistances to the input.
void EnergyHandler::DoStatusEffects(EnergyLevels &input, bool disabled) const
{
	input.shields -= input.discharge;
	input.hull -= input.corrosion;
	input.energy -= input.ionization;
	input.fuel -= input.leakage;
	input.heat += input.burn;

	auto DoResistance = [&input](bool disabled, double &stat, const EnergyLevels &cost)
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
		if(cost.fuel > 0.)
			resistance = min(resistance, input.fuel / cost.fuel);
		if(cost.heat < 0.)
			resistance = min(resistance, input.heat / -cost.heat);

		if(resistance > 0.)
		{
			stat = max(0., .99 * stat - resistance);
			input.energy -= resistance * cost.energy;
			input.fuel -= resistance * cost.fuel;
			input.heat += resistance * cost.heat;
		}
		else
			stat = max(0., .99 * stat);
	};

	DoResistance(disabled, input.corrosion, corrosionResist);
	DoResistance(disabled, input.discharge, dischargeResist);
	DoResistance(disabled, input.ionization, ionizationResist);
	DoResistance(disabled, input.burn, burnResist);
	DoResistance(disabled, input.leakage, leakageResist);
	DoResistance(disabled, input.disruption, disruptionResist);
	DoResistance(disabled, input.slowness, slownessResist);
}



// Return the amount of value that the given input can output
// given the maximum possible output and its cost.
double EnergyHandler::FractionalUsage(EnergyLevels &input, const EnergyLevels &cost, double output) const
{
	double scale = 1.;
	if(input.hull < cost.hull * scale)
		scale *= input.hull / (cost.hull * scale);
	if(input.shields < cost.shields * scale)
		scale *= input.shields / (cost.shields * scale);
	if(input.energy < cost.energy * scale)
		scale *= input.energy / (cost.energy * scale);
	if(input.heat < -cost.heat * scale)
		scale *= input.heat / (-cost.heat * scale);
	if(input.fuel < cost.fuel * scale)
		scale *= input.fuel / (cost.fuel * scale);

	return scale * output;
}



// Apply damage to the input.
void EnergyHandler::Damage(EnergyLevels &input, const EnergyLevels &damage, double scale) const
{
	input.hull -= scale * damage.hull;
	input.shields -= scale * damage.shields;
	input.energy -= scale * damage.energy;
	input.heat += scale * damage.heat;
	input.fuel -= scale * damage.fuel;

	input.corrosion += scale * damage.corrosion;
	input.discharge += scale * damage.discharge;
	input.ionization += scale * damage.ionization;
	input.burn += scale * damage.burn;
	input.leakage += scale * damage.leakage;

	input.disruption += scale * damage.disruption;
	input.slowness += scale * damage.slowness;
}



bool EnergyHandler::CanExpend(const EnergyLevels &input, const EnergyLevels &cost) const
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



// Update the stored EnergyLevels for each action a ship can take.
void EnergyHandler::ShieldRegen(const Outfit &attributes)
{
	// Save shield regen costs as per unit of shield regen.
	shieldRegenLevels.wildcard = attributes.Get("shield generation") * (1. + attributes.Get("shield generation multiplier"));
	shieldRegenLevels.energy = attributes.Get("shield energy") * (1. + attributes.Get("shield energy multiplier")) / shieldRegenLevels.wildcard;
	shieldRegenLevels.fuel = attributes.Get("shield fuel") * (1. + attributes.Get("shield fuel multiplier")) / shieldRegenLevels.wildcard;
	shieldRegenLevels.heat = attributes.Get("shield heat") * (1. + attributes.Get("shield heat multiplier")) / shieldRegenLevels.wildcard;
}



void EnergyHandler::HullRepair(const Outfit &attributes)
{
	// Save hull repair costs as per unit of hull repair.
	hullRepairLevels.wildcard = attributes.Get("hull repair rate") * (1. + attributes.Get("hull repair multiplier"));
	hullRepairLevels.energy = attributes.Get("hull energy") * (1. + attributes.Get("hull energy multiplier")) / hullRepairLevels.wildcard;
	hullRepairLevels.fuel = attributes.Get("hull fuel") * (1. + attributes.Get("hull fuel multiplier")) / hullRepairLevels.wildcard;
	hullRepairLevels.heat = attributes.Get("hull heat") * (1. + attributes.Get("hull heat multiplier")) / hullRepairLevels.wildcard;
}



void EnergyHandler::CorrosionResist(const Outfit &attributes)
{
	// Save resistance costs as per unit of resistance.
	corrosionResist.wildcard = attributes.Get("corrosion resistance");
	corrosionResist.energy = attributes.Get("corrosion resistance energy") / corrosionResist.wildcard;
	corrosionResist.fuel = attributes.Get("corrosion resistance fuel") / corrosionResist.wildcard;
	corrosionResist.heat = attributes.Get("corrosion resistance heat") / corrosionResist.wildcard;
}



void EnergyHandler::DischargeResist(const Outfit &attributes)
{
	// Save resistance costs as per unit of resistance.
	dischargeResist.wildcard = attributes.Get("discharge resistance");
	dischargeResist.energy = attributes.Get("discharge resistance energy") / dischargeResist.wildcard;
	dischargeResist.fuel = attributes.Get("discharge resistance fuel") / dischargeResist.wildcard;
	dischargeResist.heat = attributes.Get("discharge resistance heat") / dischargeResist.wildcard;
}



void EnergyHandler::IonizationResist(const Outfit &attributes)
{
	// Save resistance costs as per unit of resistance.
	ionizationResist.wildcard = attributes.Get("ion resistance");
	ionizationResist.energy = attributes.Get("ion resistance energy") / ionizationResist.wildcard;
	ionizationResist.fuel = attributes.Get("ion resistance fuel") / ionizationResist.wildcard;
	ionizationResist.heat = attributes.Get("ion resistance heat") / ionizationResist.wildcard;
}



void EnergyHandler::BurnResist(const Outfit &attributes)
{
	// Save resistance costs as per unit of resistance.
	burnResist.wildcard = attributes.Get("burn resistance");
	burnResist.energy = attributes.Get("burn resistance energy") / burnResist.wildcard;
	burnResist.fuel = attributes.Get("burn resistance fuel") / burnResist.wildcard;
	burnResist.heat = attributes.Get("burn resistance heat") / burnResist.wildcard;
}



void EnergyHandler::LeakageResist(const Outfit &attributes)
{
	// Save resistance costs as per unit of resistance.
	leakageResist.wildcard = attributes.Get("leak resistance");
	leakageResist.energy = attributes.Get("leak resistance energy") / leakageResist.wildcard;
	leakageResist.fuel = attributes.Get("leak resistance fuel") / leakageResist.wildcard;
	leakageResist.heat = attributes.Get("leak resistance heat") / leakageResist.wildcard;
}



void EnergyHandler::DisruptionResist(const Outfit &attributes)
{
	// Save resistance costs as per unit of resistance.
	disruptionResist.wildcard = attributes.Get("disruption resistance");
	disruptionResist.energy = attributes.Get("disruption resistance energy") / disruptionResist.wildcard;
	disruptionResist.fuel = attributes.Get("disruption resistance fuel") / disruptionResist.wildcard;
	disruptionResist.heat = attributes.Get("disruption resistance heat") / disruptionResist.wildcard;
}



void EnergyHandler::SlownessResist(const Outfit &attributes)
{
	// Save resistance costs as per unit of resistance.
	slownessResist.wildcard = attributes.Get("slowing resistance");
	slownessResist.energy = attributes.Get("slowing resistance energy") / slownessResist.wildcard;
	slownessResist.fuel = attributes.Get("slowing resistance fuel") / slownessResist.wildcard;
	slownessResist.heat = attributes.Get("slowing resistance heat") / slownessResist.wildcard;
}



void EnergyHandler::Thrust(const Outfit &attributes)
{
	thrustLevels.wildcard = attributes.Get("thrust");

	thrustLevels.hull = attributes.Get("thrusting hull");
	thrustLevels.shields = attributes.Get("thrusting shields");
	thrustLevels.energy = attributes.Get("thrusting energy");
	thrustLevels.heat = attributes.Get("thrusting heat");
	thrustLevels.fuel = attributes.Get("thrusting fuel");

	thrustLevels.corrosion = attributes.Get("thrusting corrosion");
	thrustLevels.discharge = attributes.Get("thrusting discharge");
	thrustLevels.ionization = attributes.Get("thrusting ion");
	thrustLevels.burn = attributes.Get("thrusting burn");
	thrustLevels.leakage = attributes.Get("thrusting leakage");
	thrustLevels.disruption = attributes.Get("thrusting disruption");
	thrustLevels.slowness = attributes.Get("thrusting slowing");
}



void EnergyHandler::ReverseThrust(const Outfit &attributes)
{
	reverseThrustLevels.wildcard = attributes.Get("reverse thrust");

	reverseThrustLevels.hull = attributes.Get("reverse thrusting hull");
	reverseThrustLevels.shields = attributes.Get("reverse thrusting shields");
	reverseThrustLevels.energy = attributes.Get("reverse thrusting energy");
	reverseThrustLevels.heat = attributes.Get("reverse thrusting heat");
	reverseThrustLevels.fuel = attributes.Get("reverse thrusting fuel");

	reverseThrustLevels.corrosion = attributes.Get("reverse thrusting corrosion");
	reverseThrustLevels.discharge = attributes.Get("reverse thrusting discharge");
	reverseThrustLevels.ionization = attributes.Get("reverse thrusting ion");
	reverseThrustLevels.burn = attributes.Get("reverse thrusting burn");
	reverseThrustLevels.leakage = attributes.Get("reverse thrusting leakage");
	reverseThrustLevels.disruption = attributes.Get("reverse thrusting disruption");
	reverseThrustLevels.slowness = attributes.Get("reverse thrusting slowing");
}



void EnergyHandler::Turn(const Outfit &attributes)
{
	turnLevels.wildcard = attributes.Get("turn");

	turnLevels.hull = attributes.Get("turning hull");
	turnLevels.shields = attributes.Get("turning shields");
	turnLevels.energy = attributes.Get("turning energy");
	turnLevels.heat = attributes.Get("turning heat");
	turnLevels.fuel = attributes.Get("turning fuel");

	turnLevels.corrosion = attributes.Get("turning corrosion");
	turnLevels.discharge = attributes.Get("turning discharge");
	turnLevels.ionization = attributes.Get("turning ion");
	turnLevels.burn = attributes.Get("turning burn");
	turnLevels.leakage = attributes.Get("turning leakage");
	turnLevels.disruption = attributes.Get("turning disruption");
	turnLevels.slowness = attributes.Get("turning slowing");
}



void EnergyHandler::Afterburner(const Outfit &attributes)
{
	afterburnerLevels.wildcard = attributes.Get("afterburner thrust");

	afterburnerLevels.hull = attributes.Get("afterburner hull");
	afterburnerLevels.shields = attributes.Get("afterburner shields");
	afterburnerLevels.energy = attributes.Get("afterburner energy");
	afterburnerLevels.heat = attributes.Get("afterburner heat");
	afterburnerLevels.fuel = attributes.Get("afterburner fuel");

	afterburnerLevels.corrosion = attributes.Get("afterburner corrosion");
	afterburnerLevels.discharge = attributes.Get("afterburner discharge");
	afterburnerLevels.ionization = attributes.Get("afterburner ion");
	afterburnerLevels.burn = attributes.Get("afterburner burn");
	afterburnerLevels.leakage = attributes.Get("afterburner leakage");
	afterburnerLevels.disruption = attributes.Get("afterburner disruption");
	afterburnerLevels.slowness = attributes.Get("afterburner slowing");
}
