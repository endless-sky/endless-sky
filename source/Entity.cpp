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

#include "Effect.h"
#include "GameData.h"
#include "image/Mask.h"
#include "Random.h"
#include "Visual.h"

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



double Entity::ShieldFraction() const
{
	double maximum = MaxShields();
	return maximum ? min(1., levels.shields / maximum) : 0.;
}



double Entity::HullFraction() const
{
	double maximum = MaxHull();
	return maximum ? min(1., levels.hull / maximum) : 1.;
}



double Entity::FuelFraction() const
{
	double maximum = capacities.fuel;
	return maximum ? min(1., levels.fuel / maximum) : 0.;
}



double Entity::EnergyFraction() const
{
	double maximum = capacities.energy;
	return maximum ? min(1., levels.energy / maximum) : (levels.hull > 0.) ? 1. : 0.;
}



double Entity::HeatFraction() const
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



double Entity::MinHull() const
{
	return minimumHull;
}



double Entity::HealthFraction() const
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



double Entity::DisabledHullFraction() const
{
	return (capacities.hull > 0. ? minimumHull / capacities.hull : 0.);
}



double Entity::HullLevelUntilDisabled() const
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



void Entity::Kill()
{
	levels.hull = -1;
	levels.shields = 0;
	levels.energy = 0.;
	levels.heat = 0.;
	levels.fuel = 0.;
	ClearStatusEffects();
}



void Entity::ClearStatusEffects()
{
	levels.discharge = 0.;
	levels.corrosion = 0.;
	levels.scrambling = 0.;
	levels.ionization = 0.;
	levels.leakage = 0.;
	levels.burning = 0.;
	levels.disruption = 0.;
	levels.slowness = 0.;
}



void Entity::DoStatusEffects(bool disabled)
{
	levels.hull -= levels.corrosion;
	levels.shields -= levels.discharge;
	levels.energy -= levels.ionization;
	levels.heat += levels.burning;
	levels.fuel -= levels.leakage;

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
			resistance = min(resistance, levels.energy / cost.energy);
		if(cost.heat < 0.)
			resistance = min(resistance, levels.heat / -cost.heat);
		if(cost.fuel > 0.)
			resistance = min(resistance, levels.fuel / cost.fuel);

		if(resistance > 0.)
		{
			stat = max(0., .99 * stat - resistance);
			levels.energy -= resistance * cost.energy;
			levels.heat += resistance * cost.heat;
			levels.fuel -= resistance * cost.fuel;
		}
		else
			stat = max(0., .99 * stat);
	};

	DoResistance(levels.corrosion, corrosionResistance, corrosionResistCost);
	DoResistance(levels.discharge, dischargeResistance, dischargeResistCost);
	DoResistance(levels.ionization, ionizationResistance, ionizationResistCost);
	DoResistance(levels.scrambling, scramblingResistance, scramblingResistCost);
	DoResistance(levels.burning, burnResistance, burnResistCost);
	DoResistance(levels.leakage, leakResistance, leakageResistCost);
	DoResistance(levels.disruption, disruptionResistance, disruptionResistCost);
	DoResistance(levels.slowness, slowingResistance, slownessResistCost);
}



void Entity::DoStatusSparks(std::vector<Visual> &visuals) const
{
	if(levels.ionization)
		CreateSparks(visuals, "ion spark", levels.ionization * .05);
	if(levels.scrambling)
		CreateSparks(visuals, "scramble spark", levels.scrambling * .05);
	if(levels.disruption)
		CreateSparks(visuals, "disruption spark", levels.disruption * .1);
	if(levels.slowness)
		CreateSparks(visuals, "slowing spark", levels.slowness * .1);
	if(levels.discharge)
		CreateSparks(visuals, "discharge spark", levels.discharge * .1);
	if(levels.corrosion)
		CreateSparks(visuals, "corrosion spark", levels.corrosion * .1);
	if(levels.leakage)
		CreateSparks(visuals, "leakage spark", levels.leakage * .1);
	if(levels.burning)
		CreateSparks(visuals, "burning spark", levels.burning * .1);
}



void Entity::CreateSparks(vector<Visual> &visuals, const string &name, double amount) const
{
	CreateSparks(visuals, GameData::Effects().Get(name), amount);
}



void Entity::CreateSparks(vector<Visual> &visuals, const Effect *effect, double amount) const
{
	if(forget || amount <= 0.)
		return;

	// Limit the number of sparks, depending on the size of the sprite.
	// The limit needs to be the first argument in case amount is NaN.
	amount = min(Width() * Height() * .0006, amount);
	// Preallocate capacity, in case we're adding a non-trivial number of sparks.
	visuals.reserve(visuals.size() + static_cast<size_t>(amount));

	while(true)
	{
		amount -= Random::Real();
		if(amount <= 0.)
			break;

		Point point((Random::Real() - .5) * Width(), (Random::Real() - .5) * Height());
		if(GetMask().Contains(point, Angle()))
			visuals.emplace_back(*effect, angle.Rotate(point) + position, velocity, angle);
	}
}



void Entity::CacheAttributes()
{
	capacities.hull = attributes.Get("hull") * (1 + attributes.Get("hull multiplier"));
	capacities.shields = attributes.Get("shields") * (1 + attributes.Get("shield multiplier"));
	capacities.energy = attributes.Get("energy capacity");
	// Heat capacity is dictated by factors other than attributes
	// and therefore isn't saved here.
	capacities.fuel = attributes.Get("fuel capacity");

	opticalJamming = attributes.Get("optical jamming");
	radarJamming = attributes.Get("radar jamming");

	auto CalibrateResistance = [this](const string &name, double &stat, ResourceLevels &cost) -> void {
		stat = attributes.Get(name + " resistance");
		// Save resistance costs as per unit of resistance.
		if(stat)
		{
			cost.energy = attributes.Get(name + " resistance energy") / stat;
			cost.heat = attributes.Get(name + " resistance heat") / stat;
			cost.fuel = attributes.Get(name + " resistance fuel") / stat;
		}
	};

	CalibrateResistance("corrosion", corrosionResistance, corrosionResistCost);
	CalibrateResistance("discharge", dischargeResistance, dischargeResistCost);
	CalibrateResistance("ion", ionizationResistance, ionizationResistCost);
	CalibrateResistance("scramble", scramblingResistance, scramblingResistCost);
	CalibrateResistance("burn", burnResistance, burnResistCost);
	CalibrateResistance("leak", leakResistance, leakageResistCost);
	CalibrateResistance("disruption", disruptionResistance, disruptionResistCost);
	CalibrateResistance("slowing", slowingResistance, slownessResistCost);
}
