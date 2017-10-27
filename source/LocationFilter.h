/* LocationFilter.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef LOCATION_FILTER_H_
#define LOCATION_FILTER_H_

#include <list>
#include <set>
#include <string>

class DataNode;
class DataWriter;
class Government;
class Planet;
class Ship;
class System;



// This class represents a set of constraints on a randomly chosen planet or
// system. For example, it can require that the planet used for a mission have
// a certain attribute or be owned by a certain government, or be a certain
// distance away from the current system.
class LocationFilter {
public:
	// Examine all the children of the given node and load any that are filters.
	void Load(const DataNode &node);
	// This only saves the children. Save the root node separately. It does
	// handle indenting, however.
	void Save(DataWriter &out) const;
	
	// Check if this filter contains any specifications.
	bool IsEmpty() const;
	
	// If the player is in the given system, does this filter match?
	bool Matches(const Planet *planet, const System *origin = nullptr) const;
	bool Matches(const System *system, const System *origin = nullptr) const;
	bool Matches(const Ship &ship) const;
	
	
private:
	// Load one particular line of conditions.
	void LoadChild(const DataNode &child);
	// Check if the filter matches the given system. If it did not, return true
	// only if the filter wasn't looking for planet characteristics or if the
	// didPlanet argument is set (meaning we already checked those).
	bool Matches(const System *system, const System *origin, bool didPlanet) const;
	
	
private:
	// The planet must satisfy these conditions:
	std::set<const Planet *> planets;
	// It must have at least one attribute from each set in this list:
	std::list<std::set<std::string>> attributes;
	
	// The system must satisfy these conditions:
	std::set<const System *> systems;
	std::set<const Government *> governments;
	const System *center = nullptr;
	int centerMinDistance = 0;
	int centerMaxDistance = 1;
	int originMinDistance = 0;
	int originMaxDistance = -1;
	
	// These filters store all the things the planet or system must not be.
	std::list<LocationFilter> notFilters;
};



#endif
