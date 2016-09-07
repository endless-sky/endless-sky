/* Person.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PERSON_H_
#define PERSON_H_

#include "LocationFilter.h"
#include "Personality.h"
#include "Phrase.h"

#include <memory>

class DataNode;
class Government;
class Ship;



// A unique individual who may appear at random times in the game.
class Person {
public:
	void Load(const DataNode &node);
	
	// Find out how often this person should appear in the given system. If this
	// person is dead or already active, this will return zero.
	int Frequency(const System *system) const;
	
	// Get the person's characteristics. The ship object is persistent, i.e. it
	// will be recycled every time this person appears.
	const std::shared_ptr<Ship> &GetShip() const;
	const Government *GetGovernment() const;
	const Personality &GetPersonality() const;
	const Phrase &GetHail() const;
	// Check if a person has been destroyed or captured.
	bool IsDestroyed() const;
	
	
private:
	LocationFilter location;
	int frequency = 100;
	
	std::shared_ptr<Ship> ship;
	const Government *government = nullptr;
	Personality personality;
	Phrase hail;
};



#endif
