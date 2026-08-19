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
	else if(key == "ionization")
		ionization = value;
	else if(key == "scrambling")
		scrambling = value;
	else if(key == "disruption")
		disruption = value;
	else if(key == "slowness")
		slowness = value;
	else if(key == "discharge")
		discharge = value;
	else if(key == "corrosion")
		corrosion = value;
	else if(key == "leakage")
		leakage = value;
	else if(key == "burning")
		burning = value;
	else
		node.PrintTrace("Skipping unrecognized attribute:");
}



void ResourceLevels::Damage(const ResourceLevels &damage, double scale)
{
	hull -= scale * damage.hull;
	shields -= scale * damage.shields;
	energy -= scale * damage.energy;
	heat += scale * damage.heat;
	fuel -= scale * damage.fuel;

	corrosion += scale * damage.corrosion;
	discharge += scale * damage.discharge;
	ionization += scale * damage.ionization;
	scrambling += scale * damage.scrambling;
	burning += scale * damage.burning;
	leakage += scale * damage.leakage;
	disruption += scale * damage.disruption;
	slowness += scale * damage.slowness;

	// Prevent various stats from reaching unallowable values.
	// Hull is allowed to go negative.
	// Shields, energy, heat, and fuel should have been prevented from going negative elsewhere.
	corrosion = max(0., corrosion);
	discharge = max(0., discharge);
	ionization = max(0., ionization);
	scrambling = max(0., scrambling);
	burning = max(0., burning);
	leakage = max(0., leakage);
	disruption = max(0., disruption);
	slowness = max(0., slowness);
}



bool ResourceLevels::CanExpend(const ResourceLevels &cost, bool includeDoT) const
{
	if(hull < cost.hull)
		return false;
	if(shields < cost.shields)
		return false;
	if(energy < cost.energy)
		return false;
	if(heat < -cost.heat)
		return false;
	if(fuel < cost.fuel)
		return false;
	if(includeDoT)
	{
		if(corrosion < -cost.corrosion)
			return false;
		if(discharge < -cost.discharge)
			return false;
		if(ionization < -cost.ionization)
			return false;
		if(burning < -cost.burning)
			return false;
		if(leakage < -cost.leakage)
			return false;
		if(disruption < -cost.disruption)
			return false;
		if(slowness < -cost.slowness)
			return false;
	}
	return true;
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
		ScaleOutput(disruption, -cost.disruption);
		ScaleOutput(slowness, -cost.slowness);
	}

	return scale;
}



ResourceLevels ResourceLevels::operator*(double scalar) const
{
	ResourceLevels levels;
	levels.hull = hull * scalar;
	levels.shields = shields * scalar;
	levels.energy = energy * scalar;
	levels.heat = heat * scalar;
	levels.fuel = fuel * scalar;
	levels.ionization = ionization * scalar;
	levels.scrambling = scrambling * scalar;
	levels.disruption = disruption * scalar;
	levels.slowness = slowness * scalar;
	levels.discharge = discharge * scalar;
	levels.corrosion = corrosion * scalar;
	levels.leakage = leakage * scalar;
	levels.burning = burning * scalar;
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
	retVal.ionization = levels.ionization * scalar;
	retVal.scrambling = levels.scrambling * scalar;
	retVal.disruption = levels.disruption * scalar;
	retVal.slowness = levels.slowness * scalar;
	retVal.discharge = levels.discharge * scalar;
	retVal.corrosion = levels.corrosion * scalar;
	retVal.leakage = levels.leakage * scalar;
	retVal.burning = levels.burning * scalar;
	return retVal;
}
