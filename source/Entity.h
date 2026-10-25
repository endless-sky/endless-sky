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

#include "DamageDealt.h"
#include "Outfit.h"
#include "ship/ResourceLevels.h"

#include <vector>

class Government;
class Visual;


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
	double ShieldFraction() const;
	double HullFraction() const;
	double FuelFraction() const;
	double EnergyFraction() const;
	double HeatFraction() const;

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
	// Get the heat dissipation, in heat units per heat unit per frame.
	double HeatDissipation() const;

	// Get the hull amount at which this entity is disabled.
	double MinHull() const;

	// Get the entity's "health," where <=0 is disabled and 1 means full health.
	double HealthFraction() const;
	// Get the hull fraction at which this ship is disabled.
	double DisabledHullFraction() const;
	// Get the (absolute) amount of hull that needs to be damaged until the
	// entity becomes disabled. Returns 0 if the entity's hull is already below the
	// disabled threshold.
	double HullLevelUntilDisabled() const;
	// Whether this entity is currently disabled or destroyed.
	virtual bool IsDisabled() const;
	bool IsDestroyed() const;

	// Whether this entity can be targeted by ships and projectiles.
	virtual bool IsTargetable() const;
	// The cloaking progress of this entity, and whether it is fully cloaked.
	virtual double Cloaking() const;
	virtual bool IsCloaked() const;

	// Attributes that influence projectiles tracking this entity.
	double OpticalSize() const;
	double OpticalJamming() const;
	double RadarJamming() const;

	// Attributes that influence how this entity takes damage.
	const ResourceLevels &DamageProtection() const;
	double PiercingProtection() const;
	double PiercingResistance() const;
	double HighShieldPermeability() const;
	double LowShieldPermeability() const;
	double CloakedShieldPermeability() const;
	double CloakedHullProtection() const;
	double CloakedShieldProtection() const;
	double ForceProtection() const;

	// Kill this entity. Set all of its resource levels to 0 and its hull to -1.
	void Kill();
	// Clear all status effects.
	void ClearStatusEffects();
	// Step all status effects and damage the entity according to any currently accumulated damage over time.
	void DoStatusEffects(bool disabled = false);

	// Create status spark visuals on the entity based on the current status effect levels.
	void DoStatusSparks(std::vector<Visual> &visuals) const;
	// Place a "spark" effect, like ionization or disruption.
	void CreateSparks(std::vector<Visual> &visuals, const std::string &name, double amount) const;
	void CreateSparks(std::vector<Visual> &visuals, const Effect *effect, double amount) const;

	// Damage this entity, creating any damage visuals in the process.
	// For ship entities, the return value is a ShipEvent type, which may be a combination of PROVOKED, DISABLED,
	// and DESTROYED.
	// For minable entities, always returns 0, as minables don't have damage events.
	int TakeDamage(std::vector<Visual> &visuals, const DamageDealt &damage, const Government *hitBy);


protected:
	// Cache commonly requested attributes into fields on the Entity-level.
	virtual void CacheAttributes();
	// TakeDamage logic that is specific to the Entity type.
	// wasDisabled and wasDestroyed record the state prior to the damage being applied.
	virtual int DoTakeDamage(const DamageDealt &damage, const Government *hitBy, bool wasDisabled,
		bool wasDestroyed) = 0;


protected:
	static constexpr double MAXIMUM_TEMPERATURE = 100.;


protected:
	Type entityType = Type::SHIP;
	Outfit attributes;

	// A counter of how many frames this entity has been out of system.
	// Individual entities may make use of this counter to call MarkForRemoval
	// if the entity has been away for too long, or entities may be removed
	// the moment they are out of the system.
	int forget = 0;

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

	double heatDissipation = 0.;

	// Attributes that influence projectiles tracking this entity.
	double opticalSize = 0.;
	double opticalJamming = 0.;
	double radarJamming = 0.;

	// Cached damage-related attributes.
	ResourceLevels damageProtection;
	double piercingProtection = 1.;
	double piercingResistance = 0.;
	double highShieldPermeability = 0.;
	double lowShieldPermeability = 0.;
	double cloakedShieldPermeability = 0.;
	double cloakedHullProtection = 0.;
	double cloakedShieldProtection = 0.;
	double forceProtection = 1.;

	// Cached status effect resistance and cost values.
	double corrosionResistance = 0.;
	ResourceLevels corrosionResistCost;
	double dischargeResistance = 0.;
	ResourceLevels dischargeResistCost;
	double ionizationResistance = 0.;
	ResourceLevels ionizationResistCost;
	double scramblingResistance = 0.;
	ResourceLevels scramblingResistCost;
	double burnResistance = 0.;
	ResourceLevels burnResistCost;
	double leakResistance = 0.;
	ResourceLevels leakageResistCost;
	double disruptionResistance = 0.;
	ResourceLevels disruptionResistCost;
	double slowingResistance = 0.;
	ResourceLevels slownessResistCost;
};
