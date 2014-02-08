/* Planet.h
Michael Zahniser, 1 Jan 2014

Class representing a stellar object you can land on. (This includes planets,
moons, and space stations.)
*/

#ifndef PLANET_H_INCLUDED
#define PLANET_H_INCLUDED

#include "DataFile.h"
#include "Sale.h"

#include <string>
#include <vector>

class Outfit;
class Ship;
class Sprite;



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
	const std::vector<const Ship *> &Shipyard() const;
	// Check if this planet has an outfitter.
	bool HasOutfitter() const;
	// Get the list of outfits available from the outfitter.
	const std::vector<const Outfit *> &Outfitter() const;
	
	
private:
	std::string name;
	std::string description;
	std::string spaceport;
	const Sprite *landscape;
	
	std::vector<const Sale<Ship> *> shipSales;
	std::vector<const Sale<Outfit> *> outfitSales;
	// The lists above will be converted into actual ship lists when they are
	// first asked for:
	mutable std::vector<const Ship *> shipyard;
	mutable std::vector<const Outfit *> outfitter;
};



#endif
