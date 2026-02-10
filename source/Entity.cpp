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
	double maximum = this->MaximumHeat();
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



double Entity::MaxShields() const
{
	return capacities.shields;
}



double Entity::MaxHull() const
{
	return capacities.hull;
}



double Entity::MinimumHull() const
{
	if(neverDisabled)
		return 0.;

	return minimumHull;
}



double Entity::Health() const
{
	double minimumHull = MinimumHull();
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
	double hull = MaxHull();
	double minimumHull = MinimumHull();

	return (hull > 0. ? minimumHull / hull : 0.);
}



double Entity::HullUntilDisabled() const
{
	// Ships become disabled when they surpass their minimum hull threshold,
	// not when they are directly on it, so account for this by adding a small amount
	// of hull above the current hull level.
	return max(0., levels.hull + 0.25 - MinimumHull());
}



bool Entity::IsDisabled() const
{
	if(!isDisabled || neverDisabled)
		return false;

	return levels.hull < MinimumHull();
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
