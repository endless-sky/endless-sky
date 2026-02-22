/* Entity.cpp
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Entity.h"

#include <algorithm>

using namespace std;



Entity::Type Entity::EntityType() const
{
	return entityType;
}



const Outfit &Entity::Attributes() const
{
	return attributes;
}



double Entity::Shields() const
{
	double maximum = MaxShields();
	return maximum ? min(1., levels.shields / maximum) : 0.;
}



double Entity::Hull() const
{
	double maximum = MaxHull();
	return maximum ? min(1., levels.hull / maximum) : 1.;
}



double Entity::Fuel() const
{
	double maximum = capacities.fuel;
	return maximum ? min(1., levels.fuel / maximum) : 0.;
}



double Entity::Energy() const
{
	double maximum = capacities.energy;
	return maximum ? min(1., levels.energy / maximum) : (levels.hull > 0.) ? 1. : 0.;
}



double Entity::Heat() const
{
	double maximum = this->MaxHeat();
	return maximum ? levels.heat / maximum : 1.;
}



double Entity::ShieldLevel() const
{
	return levels.shields;
}



double Entity::HullLevel() const
{
	return levels.hull;
}



double Entity::FuelLevel() const
{
	return levels.fuel;
}



double Entity::EnergyLevel() const
{
	return levels.energy;
}



double Entity::HeatLevel() const
{
	return levels.heat;
}



double Entity::DisruptionLevel() const
{
	return levels.disruption;
}



ResourceLevels Entity::AvailableResources() const
{
	ResourceLevels available;
	// An entity should not be able to disable itself through use of an outfit, so
	// the available hull excludes the hull necessary to remain enabled.
	available.hull = levels.hull - minimumHull;
	// The availability of all other resources is just how much this entity currently has.
	available.shields = levels.shields;
	available.energy = levels.energy;
	available.heat = levels.heat;
	available.fuel = levels.fuel;
	available.ionization = levels.ionization;
	available.scrambling = levels.scrambling;
	available.disruption = levels.disruption;
	available.slowness = levels.slowness;
	available.discharge = levels.discharge;
	available.corrosion = levels.corrosion;
	available.leakage = levels.leakage;
	available.burning = levels.burning;
	return available;
}



double Entity::MaxShields() const
{
	return capacities.shields;
}



double Entity::MaxHull() const
{
	return capacities.hull;
}



double Entity::MaxEnergy() const
{
	return capacities.energy;
}



double Entity::MaxFuel() const
{
	return capacities.fuel;
}



double Entity::MinimumHull() const
{
	return minimumHull;
}



double Entity::Health() const
{
	double hullDivisor = MaxHull() - minimumHull;
	double divisor = MaxShields() + hullDivisor;
	// This should not happen, but just in case.
	if(divisor <= 0. || hullDivisor <= 0.)
		return 0.;

	double spareHull = levels.hull - minimumHull;
	// Consider hull-only and pooled health, compensating for any reductions by disruption damage.
	return min(spareHull / hullDivisor, (spareHull + levels.shields / (1. + levels.disruption * .01)) / divisor);
}



double Entity::DisabledHull() const
{
	return (capacities.hull > 0. ? minimumHull / capacities.hull : 0.);
}



double Entity::HullUntilDisabled() const
{
	// Ships become disabled when they surpass their minimum hull threshold,
	// not when they are directly on it, so account for this by adding a small amount
	// of hull above the current hull level.
	return max(0., levels.hull + 0.25 - minimumHull);
}



bool Entity::IsDisabled() const
{
	if(!isDisabled || neverDisabled)
		return false;

	return levels.hull < minimumHull;
}



bool Entity::IsTargetable() const
{
	return true;
}



double Entity::OpticalJamming() const
{
	return opticalJamming;
}



double Entity::RadarJamming() const
{
	return radarJamming;
}



void Entity::CacheAttributes()
{
	opticalJamming = attributes.Get("optical jamming");
	radarJamming = attributes.Get("radar jamming");
}
