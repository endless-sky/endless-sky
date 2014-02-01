/* Planet.h
Michael Zahniser, 1 Jan 2014

Class representing a stellar object you can land on. (This includes planets,
moons, and space stations.)
*/

#ifndef PLANET_H_INCLUDED
#define PLANET_H_INCLUDED

#include "DataFile.h"

#include <string>

class Sprite;



class Planet {
public:
	// Default constructor.
	Planet();
	
	// Load a planet's description from a file.
	void Load(const DataFile::Node &node);
	
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
	
	
private:
	std::string name;
	std::string description;
	std::string spaceport;
	const Sprite *landscape;
};



#endif
