/* Entity.h
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

#pragma once

#include "Body.h"

#include "Outfit.h"
#include "ship/ResourceLevels.h"



// A class containing common elements for objects like ships and minable asteroids.
class Entity : public Body {
public:
	enum class Type {
		SHIP,
		MINABLE
	};


public:
	virtual ~Entity() {};

	// What type of entity this is.
	Type EntityType() const;

	// Get the current attributes of this entity.
	const Outfit &Attributes() const;

	// The current mass of this entity.
	virtual double Mass() const = 0;

	// Get resource levels of this entity, as a fraction between 0 and 1.
	// Values can be greater than 1 in certain cases, such as heat when overheated.
	double Shields() const;
	double Hull() const;
	double Fuel() const;
	double Energy() const;
	double Heat() const;

	// Get the absolute resource levels of the entity.
	double ShieldLevel() const;
	double HullLevel() const;
	double FuelLevel() const;
	double EnergyLevel() const;
	double HeatLevel() const;
	double DisruptionLevel() const;
	// Get the resource levels available for use from this entity.
	ResourceLevels AvailableResources() const;

	// Get the maximum resource level values of the entity.
	double MaxShields() const;
	double MaxHull() const;
	double MaxEnergy() const;
	double MaxFuel() const;
	// Get the maximum heat level, in heat units (not temperature).
	virtual double MaxHeat() const = 0;

	// Get the hull amount at which this entity is disabled.
	double MinimumHull() const;

	// Get the entity's "health," where <=0 is disabled and 1 means full health.
	double Health() const;
	// Get the hull fraction at which this ship is disabled.
	double DisabledHull() const;
	// Get the (absolute) amount of hull that needs to be damaged until the
	// entity becomes disabled. Returns 0 if the entity's hull is already below the
	// disabled threshold.
	double HullUntilDisabled() const;
	// Whether this entity is currently disabled.
	virtual bool IsDisabled() const;

	// Whether this entity can be targeted by ships and projectiles.
	virtual bool IsTargetable() const;

	// Jamming attributes that influence projectiles tracking this entity.
	double OpticalJamming() const;
	double RadarJamming() const;


protected:
	// Cache commonly requested attributes into fields on the Entity-level.
	virtual void CacheAttributes();


protected:
	static constexpr double MAXIMUM_TEMPERATURE = 100.;


protected:
	Type entityType = Type::SHIP;
	Outfit attributes;

	// The current resource levels of this entity.
	ResourceLevels levels;
	// The maximum capacities of the resource levels of this entity.
	// It is up to the individual entity to set its capacities, and not all
	// resources have a capacity.
	ResourceLevels capacities;
	// The minimum hull of this entity before it is considered disabled.
	double minimumHull = 0.;

	// Whether this entity is allowed to become disabled, and if it is disabled now.
	bool neverDisabled = false;
	bool isDisabled = false;

	// Jamming attributes that influence projectiles tracking this entity.
	double opticalJamming = 0.;
	double radarJamming = 0.;
};
