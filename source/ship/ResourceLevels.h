/* ResourceLevels.h
Copyright (c) 2025 by Amazinite

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

#include <optional>

class DataNode;
class Weapon;



// A class representing the magnitude of various resources
// that a ship has, including a ship's HP, energy, heat, fuel, and
// the amounts of various DoT applied to the ship. Resources can
// represent the values currently on a ship or the changes to be
// applied to a ship, such as an amount of damage to be taken or the
// resources required for repairs or movement.
class ResourceLevels {
public:
	enum class MissingResource {
		HULL,
		SHIELD,
		ENERGY,
		HEAT,
		FUEL,
		CORROSION,
		DISCHARGE,
		ION,
		BURN,
		LEAK,
		SCRAMBLE,
		DISRUPTION,
		SLOWNESS,
	};


public:
	ResourceLevels() = default;
	explicit ResourceLevels(const DataNode &node);
	void Load(const DataNode &node);
	void LoadSingle(const DataNode &node);

	// Receive damage. Shields, hull, energy, and fuel are subtracted. All other resources are added.
	void Damage(const ResourceLevels &damage, double scale = 1.);
	// Repair the given stat up to the maximum that the ship is capable of given the cost.
	// Updates the available variable with the remaining amount of repairs that
	// can be done.
	void DoRepair(double &stat, double &available, double maximum, const ResourceLevels &cost);

	// Return true if this object has the resources to expend on the entire cost.
	bool CanExpend(const ResourceLevels &cost, bool includeDoT = false) const;
	// Return nullopt if this object has the resources to expend on the entire cost.
	// Otherwise, return the first missing resource.
	std::optional<MissingResource> IsMissing(const ResourceLevels &cost, bool includeDoT = false) const;
	// Return the fraction of 100% output that these resources can manage given the cost.
	double FractionalUsage(const ResourceLevels &cost, bool includeDoT = false) const;
	// Return a multiple of how many times these resources could use the given cost.
	double MultipleUsage(const ResourceLevels &cost, bool includeDoT = false) const;

	// Return the firing cost of this weapon, given that this ResourceLevels contains the
	// capacities of a ship for use in calculating relative damage values.
	ResourceLevels FiringCost(const Weapon &weapon) const;
	// Return nullopt if the ship has the resources to expend on the firing cost.
	// Otherwise, return the first missing resource.
	// This ignores any shield costs, allowing ships to fire a weapon even with
	// no shields.
	std::optional<MissingResource> CanFire(const Weapon &weapon, const ResourceLevels &capacities) const;

	ResourceLevels operator*(double scalar) const;
	friend ResourceLevels operator*(double scalar, const ResourceLevels &levels);


public:
	double hull = 0.;
	double shields = 0.;
	double energy = 0.;
	double heat = 0.;
	double fuel = 0.;

	// Accrued "corrosion damage" that will affect this ship's hull over time.
	double corrosion = 0.;
	// Accrued "discharge damage" that will affect this ship's shields over time.
	double discharge = 0.;
	// Accrued "ion damage" that will affect this ship's energy over time.
	double ionization = 0.;
	// Accrued "burn damage" that will affect this ship's heat over time.
	double burning = 0.;
	// Accrued "leak damage" that will affect this ship's fuel over time.
	double leakage = 0.;

	// Accrued "scrambling damage" that will affect this ship's weaponry over time.
	double scrambling = 0.;
	// Accrued "disruption damage" that will affect this ship's shield effectiveness over time.
	double disruption = 0.;
	// Accrued "slowing damage" that will affect this ship's movement over time.
	double slowness = 0.;
};
