/* Fleet.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FLEET_H_
#define FLEET_H_

#include "Personality.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

class DataNode;
class Government;
class Phrase;
class Planet;
class Ship;
class System;



// A fleet represents a collection of ships that may enter a system or be used
// as NPCs in a mission. Each fleet contains one or more "variants," each of
// which can occur with a different probability, and each of those variants
// lists one or more ships. All the ships in a fleet share a certain government,
// AI personality, and set of friendly and hostile "hail" messages, and the ship
// names are chosen based on a given random "phrase" generator.
class Fleet {
public:
	void Load(const DataNode &node);
	
	// Get the government of this fleet.
	const Government *GetGovernment() const;
	
	void Enter(const System &system, std::list<std::shared_ptr<Ship>> &ships, const Planet *planet = nullptr) const;
	// Place a fleet in the given system, already "in action."
	void Place(const System &system, std::list<std::shared_ptr<Ship>> &ships, bool carried = true) const;
	
	// Do the randomization to make a ship enter or be in the given system.
	static void Enter(const System &system, Ship &ship);
	static void Place(const System &system, Ship &ship);
	
	int64_t Strength() const;
	
	
private:
	class Variant {
	public:
		explicit Variant(const DataNode &node);
		
		int weight;
		std::vector<const Ship *> ships;
	};
	
	
private:
	const Variant &ChooseVariant() const;
	static Point ChooseCenter(const System &system);
	std::vector<std::shared_ptr<Ship>> Instantiate(const Variant &variant) const;
	bool PlaceFighter(std::shared_ptr<Ship> fighter, std::vector<std::shared_ptr<Ship>> &placed) const;
	void SetCargo(Ship *ship) const;
	
	
private:
	std::string fleetName;
	const Government *government = nullptr;
	const Phrase *names = nullptr;
	const Phrase *fighterNames = nullptr;
	std::vector<Variant> variants;
	int cargo = 3;
	std::vector<std::string> commodities;
	int total = 0;
	
	Personality personality;
};



#endif
