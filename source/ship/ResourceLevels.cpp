/* ResourceLevels.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ResourceLevels.h"

#include "../DataNode.h"
#include "../Weapon.h"

#include <limits>
#include <string>

using namespace std;



ResourceLevels::ResourceLevels(const DataNode &node)
{
	Load(node);
}



void ResourceLevels::Load(const DataNode &node)
{
	for(const DataNode &child : node)
		LoadSingle(child);
}



void ResourceLevels::LoadSingle(const DataNode &node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("Expected key to have a value:");
		return;
	}
	const string &key = node.Token(0);
	double value = node.Value(1);
	if(key == "hull")
		hull = value;
	else if(key == "shields")
		shields = value;
	else if(key == "energy")
		energy = value;
	else if(key == "heat")
		heat = value;
	else if(key == "fuel")
		fuel = value;
	else if(key == "corrosion")
		corrosion = value;
	else if(key == "discharge")
		discharge = value;
	else if(key == "ionization")
		ionization = value;
	else if(key == "burning")
		burning = value;
	else if(key == "leakage")
		leakage = value;
	else if(key == "scrambling")
		scrambling = value;
	else if(key == "disruption")
		disruption = value;
	else if(key == "slowness")
		slowness = value;
	else
		node.PrintTrace("Skipping unrecognized attribute:");
}



void ResourceLevels::Damage(const ResourceLevels &damage, double scale)
{
	// Prevent various stats from reaching unallowable values.
	// Hull is allowed to go negative.
	hull -= scale * damage.hull;
	shields = max(0., shields - scale * damage.shields);
	energy = max(0., energy - scale * damage.energy);
	heat = max(0., heat + scale * damage.heat);
	fuel = max(0., fuel - scale * damage.fuel);

	corrosion = max(0., corrosion + scale * damage.corrosion);
	discharge = max(0., discharge + scale * damage.discharge);
	ionization = max(0., ionization + scale * damage.ionization);
	burning = max(0., burning + scale * damage.burning);
	leakage = max(0., leakage + scale * damage.leakage);

	scrambling = max(0., scrambling + scale * damage.scrambling);
	disruption = max(0., disruption + scale * damage.disruption);
	slowness = max(0., slowness + scale * damage.slowness);
}



void ResourceLevels::DoRepair(double &stat, double &available, double maximum, const ResourceLevels &cost)
{
	if(available <= 0. || stat >= maximum)
		return;

	if(cost.energy > 0.)
		available = min(available, energy / cost.energy);
	if(cost.heat < 0.)
		available = min(available, heat / -cost.heat);
	if(cost.fuel > 0.)
		available = min(available, fuel / cost.fuel);

	double transfer = min(available, maximum - stat);
	if(transfer > 0.)
	{
		stat += transfer;
		available -= transfer;
		energy -= transfer * cost.energy;
		heat += transfer * cost.heat;
		fuel -= transfer * cost.fuel;
	}
}



bool ResourceLevels::CanExpend(const ResourceLevels &cost, bool includeDoT) const
{
	return !IsMissing(cost, includeDoT);
}



optional<ResourceLevels::MissingResource> ResourceLevels::IsMissing(const ResourceLevels &cost, bool includeDoT) const
{
	if(hull < cost.hull)
		return MissingResource::HULL;
	if(shields < cost.shields)
		return MissingResource::SHIELD;
	if(energy < cost.energy)
		return MissingResource::ENERGY;
	if(heat < -cost.heat)
		return MissingResource::HEAT;
	if(fuel < cost.fuel)
		return MissingResource::FUEL;
	if(includeDoT)
	{
		if(corrosion < -cost.corrosion)
			return MissingResource::CORROSION;
		if(discharge < -cost.discharge)
			return MissingResource::DISCHARGE;
		if(ionization < -cost.ionization)
			return MissingResource::ION;
		if(burning < -cost.burning)
			return MissingResource::BURN;
		if(leakage < -cost.leakage)
			return MissingResource::LEAK;

		if(scrambling < -cost.scrambling)
			return MissingResource::SCRAMBLE;
		if(disruption < -cost.disruption)
			return MissingResource::DISRUPTION;
		if(slowness < -cost.slowness)
			return MissingResource::SLOWNESS;
	}
	return std::nullopt;
}



double ResourceLevels::FractionalUsage(const ResourceLevels &cost, bool includeDoT) const
{
	double scale = 1.;
	auto ScaleOutput = [&scale](double input, double levelCost)
	{
		if(input < levelCost * scale)
			scale = input / levelCost;
	};
	ScaleOutput(hull, cost.hull);
	ScaleOutput(shields, cost.shields);
	ScaleOutput(energy, cost.energy);
	ScaleOutput(heat, -cost.heat);
	ScaleOutput(fuel, cost.fuel);
	if(includeDoT)
	{
		ScaleOutput(corrosion, -cost.corrosion);
		ScaleOutput(discharge, -cost.discharge);
		ScaleOutput(ionization, -cost.ionization);
		ScaleOutput(burning, -cost.burning);
		ScaleOutput(leakage, -cost.leakage);

		ScaleOutput(scrambling, -cost.scrambling);
		ScaleOutput(disruption, -cost.disruption);
		ScaleOutput(slowness, -cost.slowness);
	}
	return scale;
}



double ResourceLevels::MultipleUsage(const ResourceLevels &cost, bool includeDoT) const
{
	double scale = numeric_limits<double>::infinity();
	auto ScaleOutput = [&scale](double input, double levelCost)
	{
		if(levelCost > 0.)
			scale = min(scale, input / levelCost);
	};
	ScaleOutput(hull, cost.hull);
	ScaleOutput(shields, cost.shields);
	ScaleOutput(energy, cost.energy);
	ScaleOutput(heat, -cost.heat);
	ScaleOutput(fuel, cost.fuel);
	if(includeDoT)
	{
		ScaleOutput(corrosion, -cost.corrosion);
		ScaleOutput(discharge, -cost.discharge);
		ScaleOutput(ionization, -cost.ionization);
		ScaleOutput(burning, -cost.burning);
		ScaleOutput(leakage, -cost.leakage);

		ScaleOutput(scrambling, -cost.scrambling);
		ScaleOutput(disruption, -cost.disruption);
		ScaleOutput(slowness, -cost.slowness);
	}
	return scale;
}



ResourceLevels ResourceLevels::FiringCost(const Weapon &weapon) const
{
	ResourceLevels cost;
	cost.hull = weapon.FiringHull() + weapon.RelativeFiringHull() * hull;
	cost.shields = weapon.FiringShields() + weapon.RelativeFiringShields() * shields;
	cost.energy = weapon.FiringEnergy() + weapon.RelativeFiringEnergy() * energy;
	cost.heat = weapon.FiringHeat() + weapon.RelativeFiringHeat() * heat;
	cost.fuel = weapon.FiringFuel() + weapon.RelativeFiringFuel() * fuel;

	cost.corrosion = weapon.FiringCorrosion();
	cost.discharge = weapon.FiringDischarge();
	cost.ionization = weapon.FiringIon();
	cost.burning = weapon.FiringBurn();
	cost.leakage = weapon.FiringLeak();

	cost.scrambling = weapon.FiringScramble();
	cost.disruption = weapon.FiringDisruption();
	cost.slowness = weapon.FiringSlowing();
	return cost;
}



optional<ResourceLevels::MissingResource> ResourceLevels::CanFire(const Weapon &weapon,
	const ResourceLevels &capacities) const
{
	ResourceLevels cost = capacities.FiringCost(weapon);
	// Weapons are allowed to fully strip the shields of the ship they're on, so treat
	// them as if they have a firing shield cost of 0.
	cost.shields = 0.;
	// Weapons use DoT counters as firing costs.
	return IsMissing(cost, true);
}



ResourceLevels ResourceLevels::operator*(double scalar) const
{
	ResourceLevels levels;
	levels.hull = hull * scalar;
	levels.shields = shields * scalar;
	levels.energy = energy * scalar;
	levels.heat = heat * scalar;
	levels.fuel = fuel * scalar;

	levels.corrosion = corrosion * scalar;
	levels.discharge = discharge * scalar;
	levels.ionization = ionization * scalar;
	levels.burning = burning * scalar;
	levels.leakage = leakage * scalar;

	levels.scrambling = scrambling * scalar;
	levels.disruption = disruption * scalar;
	levels.slowness = slowness * scalar;
	return levels;
}



ResourceLevels operator*(double scalar, const ResourceLevels &levels)
{
	ResourceLevels retVal;
	retVal.hull = levels.hull * scalar;
	retVal.shields = levels.shields * scalar;
	retVal.energy = levels.energy * scalar;
	retVal.heat = levels.heat * scalar;
	retVal.fuel = levels.fuel * scalar;

	retVal.corrosion = levels.corrosion * scalar;
	retVal.discharge = levels.discharge * scalar;
	retVal.ionization = levels.ionization * scalar;
	retVal.burning = levels.burning * scalar;
	retVal.leakage = levels.leakage * scalar;

	retVal.scrambling = levels.scrambling * scalar;
	retVal.disruption = levels.disruption * scalar;
	retVal.slowness = levels.slowness * scalar;
	return retVal;
}
