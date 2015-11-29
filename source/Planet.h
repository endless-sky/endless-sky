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

#include "Sale.h"

#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

class DataNode;
class Fleet;
class Outfit;
class PlayerInfo;
class Ship;
class Sprite;
class System;



// Class representing a stellar object you can land on. (This includes planets,
// moons, and space stations.) Each planet has a certain set of services that
// are available, as well as attributes that determine what sort of missions
// might choose it as a source or destination.
class Planet {
public:
	// Load a planet's description from a file.
	void Load(const DataNode &node, const Set<Sale<Ship>> &ships, const Set<Sale<Outfit>> &outfits);
	
	// Get the name of the planet.
	const std::string &Name() const;
	// Get the planet's descriptive text.
	const std::string &Description() const;
	// Get the landscape sprite.
	const Sprite *Landscape() const;
	
	// Get the list of "attributes" of the planet.
	const std::set<std::string> &Attributes() const;
	
	// Get planet's noun descriptor from attributes
	const std::string &Noun() const;
	
	// Check whether there is a spaceport (which implies there is also trading,
	// jobs, banking, and hiring).
	bool HasSpaceport() const;
	// Get the spaceport's descriptive text.
	const std::string &SpaceportDescription() const;
	
	// Check if this planet is inhabited (i.e. it has a spaceport, and does not
	// have the "uninhabited" attribute).
	bool IsInhabited() const;
	
	// Check if this planet has a shipyard.
	bool HasShipyard() const;
	// Get the list of ships in the shipyard.
	const Sale<Ship> &Shipyard() const;
	// Check if this planet has an outfitter.
	bool HasOutfitter() const;
	// Get the list of outfits available from the outfitter.
	const Sale<Outfit> &Outfitter() const;
	
	// You need this good a reputation with this system's government to land here.
	double RequiredReputation() const;
	// This is what fraction of your fleet's value you must pay as a bribe in
	// order to land on this planet. (If zero, you cannot bribe it.)
	double GetBribeFraction() const;
	// This is how likely the planet's authorities are to notice if you are
	// doing something illegal.
	double Security() const;
	
	// Set or get what system this planet is in. This is so that missions, for
	// example, can just hold a planet pointer instead of a system as well.
	const System *GetSystem() const;
	void SetSystem(const System *system);
	// Remove the given system from the list of systems this planet is in. This
	// must be done when game events rearrange the planets in a system.
	void RemoveSystem(const System *system);
	
	// Check if this is a wormhole (that is, it appears in multiple systems).
	bool IsWormhole() const;
	const System *WormholeDestination(const System *from) const;
	
	// Below are convenience functions which access the game state in Politics,
	// but do so with a less convoluted syntax:
	bool CanLand(const Ship &ship) const;
	bool CanLand() const;
	bool CanUseServices() const;
	void Bribe(bool fullAccess = true) const;
	
	// Demand tribute, and get the planet's response.
	std::string DemandTribute(PlayerInfo &player) const;
	void DeployDefense(std::list<std::shared_ptr<Ship>> &ships) const;
	void ResetDefense() const;
	
	
private:
	std::string name;
	std::string description;
	std::string spaceport;
	const Sprite *landscape = nullptr;
	
	std::set<std::string> attributes;
	
	std::vector<const Sale<Ship> *> shipSales;
	std::vector<const Sale<Outfit> *> outfitSales;
	// The lists above will be converted into actual ship lists when they are
	// first asked for:
	mutable Sale<Ship> shipyard;
	mutable Sale<Outfit> outfitter;
	
	double requiredReputation = 0.;
	double bribe = 0.01;
	double security = .25;
	
	int tribute = 0;
	const Fleet *defenseFleet = nullptr;
	int defenseCount = 0;
	mutable int defenseDeployed = 0;
	int defenseThreshold = 4000;
	mutable bool isDefending = false;
	mutable std::list<std::shared_ptr<Ship>> defenders;
	
	std::vector<const System *> systems;
};



#endif
