/* Mission.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MISSION_H_
#define MISSION_H_

#include "Conversation.h"

#include <set>
#include <string>

class DataNode;
class DataWriter;
class DistanceMap;
class Planet;



class Mission {
public:
	static Mission Cargo(const Planet *source, const DistanceMap &distance);
	static Mission Passenger(const Planet *source, const DistanceMap &distance);
	
	
public:
	void Load(const DataNode &node);
	void Save(DataWriter &out, const std::string &tag = "mission") const;
	
	const std::string &Name() const;
	const Planet *Destination() const;
	const std::string &Description() const;
	
	bool IsAvailableAt(const Planet *planet) const;
	const Conversation &Introduction() const;
	
	const std::string &Cargo() const;
	int CargoSize() const;
	int Passengers() const;
	int Payment() const;
	
	const std::string &SuccessMessage() const;
	const Mission *Next() const;
	
	
private:
	std::string name;
	const Planet *destination = nullptr;
	std::string description;
	
	std::set<const Planet *> sourcePlanets;
	Conversation introduction;
	
	std::string cargo;
	int cargoSize = 0;
	int passengers = 0;
	int payment = 0;
	
	std::string successMessage;
	const Mission *next = nullptr;
};



#endif
