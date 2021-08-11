/* Wormhole.h
Copyright (c) 2021 quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WORMHOLE_H_
#define WORMHOLE_H_

#include <string>
#include <unordered_map>
#include <vector>

class DataNode;
class Planet;
class System;



// Class representing a wormhole in a planet.
class Wormhole {
public:
	static void GenerateFromPlanet(Wormhole *wormhole, const Planet *planet);


public:
	// Load a wormhole's description from a file.
	void Load(const DataNode &node);
	// Check if this wormhole has been defined.
	bool IsValid() const;

	// Returns the planet corresponding to this wormhole.
	const Planet *GetPlanet() const;
	// Returns this wormhole's name.
	const std::string &Name() const;
	// Whether this wormhole's link appears on the map.
	bool IsLinked() const;
	
	const System *WormholeSource(const System *to) const;
	const System *WormholeDestination(const System *from) const;
	const std::unordered_map<const System *, const System *> &Links() const;

	// Updates this wormhole if the properties of the parent planet changed.
	void UpdateFromPlanet();
	
	
private:
	bool isDefined = false;
	
	const Planet *planet;
	std::string name = "???";
	bool linked = false;
	std::unordered_map<const System *, const System *> links;
};



#endif
