/* System.h
Michael Zahniser, 1 Jan 2014

Class representing a star system, including the mathematics for calculating the
star and planet positions on a given date.
*/

#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#include "Animation.h"
#include "DataFile.h"
#include "Point.h"
#include "Set.h"
#include "StellarObject.h"

class Date;
class Government;
class Planet;

#include <string>
#include <vector>



class System {
public:
	class Asteroid {
	public:
		Asteroid(const std::string &name, int count, double energy);
		
		const std::string &Name() const;
		int Count() const;
		double Energy() const;
		
	private:
		std::string name;
		int count;
		double energy;
	};
	
	
public:
	System();
	
	// Load a system's description.
	void Load(const DataFile::Node &node, const Set<System> &systems, const Set<Planet> &planets, const Set<Government> &governments);
	// Once the star map is fully loaded, figure out which stars are "neighbors"
	// of this one, i.e. close enough to see or to reach via jump drive.
	void UpdateNeighbors(const Set<System> &systems);
	
	// Get this system's name and position (in the star map).
	const std::string &Name() const;
	const Point &Position() const;
	// Get this system's government.
	const Government &GetGovernment() const;
	
	// Get a list of systems you can travel to through hyperspace from here.
	const std::vector<const System *> &Links() const;
	// Get a list of systems you can "see" from here, whether or not there is a
	// direct hyperspace link to them. This is also the set of systems that you
	// can travel to from here via the jump drive.
	const std::vector<const System *> &Neighbors() const;
	
	// Move the stellar objects to their positions on the given date.
	void SetDate(const Date &date) const;
	// Get the stellar object locations on the most recently set date.
	const std::vector<StellarObject> &Objects() const;
	// Get the habitable zone's center.
	double HabitableZone() const;
	// Check if this system is inhabited.
	bool IsInhabited() const;
	// Check whether you can buy or sell ships in this system.
	bool HasShipyard() const;
	// Check whether you can buy or sell ship outfits in this system.
	bool HasOutfitter() const;
	
	// Get the specification of how many asteroids of each type there are.
	const std::vector<Asteroid> &Asteroids() const;
	
	// Get the price of the given commodity in this system.
	int Trade(const std::string &commodity) const;
	
	
private:
	void LoadObject(const DataFile::Node &node, const Set<Planet> &planets, int parent = -1);
	
	
private:
	// Name and position (within the star map) of this system.
	std::string name;
	Point position;
	const Government *government;
	// Hyperspace links to other systems.
	std::vector<const System *> links;
	std::vector<const System *> neighbors;
	
	// Stellar objects, listed in such an order that an object's parents are
	// guaranteed to appear before it (so that if we traverse the vector in
	// order, updating positions, an object's parents will already be at the
	// proper position before that object is updated).
	std::vector<StellarObject> objects;
	std::vector<Asteroid> asteroids;
	double habitable;
	
	// Commodity prices.
	std::map<std::string, int> trade;
};



#endif
