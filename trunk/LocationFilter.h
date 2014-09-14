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
class Government;
class Planet;
class System;



class LocationFilter {
public:
	// There is no need to save a location filter, because any mission that is
	// in the saved game will already have "applied" the filter to choose a
	// particular planet or system.
	void Load(const DataNode &node);
	
	// If the player is in the given system, does this filter match?
	bool Matches(const Planet *planet, const System *origin = nullptr) const;
	bool Matches(const System *system, const System *origin = nullptr) const;
	
	
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
};



#endif
