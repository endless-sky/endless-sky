/* Planet.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PLANET_H_
#define PLANET_H_

#include "DataFile.h"
#include "Sale.h"

#include <string>
#include <vector>

class Outfit;
class Ship;
class Sprite;



// Class representing a stellar object you can land on. (This includes planets,
// moons, and space stations.)
class Planet {
public:
	// Default constructor.
	Planet();
	
	// Load a planet's description from a file.
	void Load(const DataFile::Node &node, const Set<Sale<Ship>> &ships, const Set<Sale<Outfit>> &outfits);
	
	// Get the name of the planet.
	const std::string &Name() const;
	// Get the planet's descriptive text.
	const std::string &Description() const;
	// Get the landscape sprite.
	const Sprite *Landscape() const;
	
	// Check whether there is a spaceport (which implies there is also trading,
	// jobs, banking, and hiring).
	bool HasSpaceport() const;
	// Get the spaceport's descriptive text.
	const std::string &SpaceportDescription() const;
	
	// Check if this planet has a shipyard.
	bool HasShipyard() const;
	// Get the list of ships in the shipyard.
	const Sale<Ship> &Shipyard() const;
	// Check if this planet has an outfitter.
	bool HasOutfitter() const;
	// Get the list of outfits available from the outfitter.
	const Sale<Outfit> &Outfitter() const;
	
	
private:
	std::string name;
	std::string description;
	std::string spaceport;
	const Sprite *landscape;
	
	std::vector<const Sale<Ship> *> shipSales;
	std::vector<const Sale<Outfit> *> outfitSales;
	// The lists above will be converted into actual ship lists when they are
	// first asked for:
	mutable Sale<Ship> shipyard;
	mutable Sale<Outfit> outfitter;
};



#endif
