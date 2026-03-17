/* LocationFilter.h
Copyright (c) 2014 by Michael Zahniser

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

#include "DistanceCalculationSettings.h"

#include <list>
#include <set>
#include <string>

class DataNode;
class DataWriter;
class Government;
class Outfit;
class Planet;
class Ship;
class System;



// This class represents a set of constraints on a randomly chosen ship, planet,
// or system. For example, it can require that the planet used for a mission
// have a certain attribute or be owned by a certain government, or be a
// certain distance away from the current system.
class LocationFilter {
public:
	LocationFilter() noexcept = default;
	// Construct and Load() at the same time.
	explicit LocationFilter(const DataNode &node, const std::set<const System *> *visitedSystems,
		const std::set<const Planet *> *visitedPlanets);

	// Examine all the children of the given node and load any that are filters.
	void Load(const DataNode &node, const std::set<const System *> *visitedSystems,
		const std::set<const Planet *> *visitedPlanets);
	// This only saves the children. Save the root node separately. It does
	// handle indenting, however.
	void Save(DataWriter &out) const;

	// Check if this filter contains any specifications.
	bool IsEmpty() const;
	bool IsValid() const;

	// If the player is in the given system, does this filter match?
	bool Matches(const Planet *planet, const System *origin = nullptr) const;
	bool Matches(const System *system, const System *origin = nullptr) const;
	// Ships are chosen based on system/"near" filters, government, category
	// of ship, outfits installed/carried, and their total attributes.
	bool Matches(const Ship &ship) const;

	// Return a new LocationFilter with any "distance" conditions converted
	// into "near" references, relative to the given system.
	LocationFilter SetOrigin(const System *origin) const;
	// Generic find system / find planet methods, based on the given origin
	// system (e.g. the player's current system) and ability to land.
	const System *PickSystem(const System *origin) const;
	const Planet *PickPlanet(const System *origin, bool hasClearance = false, bool requireSpaceport = true) const;


private:
	// Load one particular line of conditions.
	void LoadChild(const DataNode &child, const std::set<const System *> *visitedSystems,
		const std::set<const Planet *> *visitedPlanets);
	// Check if the filter matches the given system. If it did not, return true
	// only if the filter wasn't looking for planet characteristics or if the
	// didPlanet argument is set (meaning we already checked those).
	bool Matches(const System *system, const System *origin, bool didPlanet) const;


private:
	bool isEmpty = true;

	// Pointers to the PlayerInfo's visited systems and planets.
	const std::set<const System *> *visitedSystems = nullptr;
	const std::set<const Planet *> *visitedPlanets = nullptr;

	// The player must have visited the system or planet.
	bool systemIsVisited = false;
	bool planetIsVisited = false;

	// The planet must satisfy these conditions:
	std::set<const Planet *> planets;
	// It must have at least one attribute from each set in this list:
	std::list<std::set<std::string>> attributes;

	// The system must satisfy these conditions:
	std::set<const System *> systems;
	std::set<const Government *> governments;
	// The reference point and distance limits of a "near <system>" filter.
	const System *center = nullptr;
	int centerMinDistance = 0;
	int centerMaxDistance = 1;
	DistanceCalculationSettings centerDistanceOptions;
	// Distance limits used in a "distance" filter.
	int originMinDistance = 0;
	int originMaxDistance = -1;
	DistanceCalculationSettings originDistanceOptions;

	// At least one of the outfits from each set must be available
	// (to purchase or plunder):
	std::list<std::set<const Outfit *>> outfits;
	// A ship must belong to one of these categories:
	std::set<std::string> shipCategory;

	// These filters store all the things the planet, system, or ship must not be.
	std::list<LocationFilter> notFilters;
	// These filters store all the things the planet or system must border.
	std::list<LocationFilter> neighborFilters;
};
