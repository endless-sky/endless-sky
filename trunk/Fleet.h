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

#include "DataFile.h"

#include <list>
#include <memory>
#include <vector>

class GameData;
class Government;
class Ship;
class ShipName;
class System;



class Fleet {
public:
	Fleet();
	
	void Load(const DataFile::Node &node, const GameData &data);
	
	void Enter(const System &system, std::list<std::shared_ptr<Ship>> &ships) const;
	// Place a fleet in the given system, already "in action."
	void Place(const System &system, std::list<std::shared_ptr<Ship>> &ships) const;
	
	
private:
	void SetCargo(Ship *ship) const;
	
	
private:
	class Variant {
	public:
		Variant(const DataFile::Node &node, const GameData &data);
		
		int weight;
		std::vector<const Ship *> ships;
	};
	
	
private:
	const GameData *data;
	
	const Government *government;
	const ShipName *names;
	std::vector<Variant> variants;
	int cargo;
	int total;
};



#endif
