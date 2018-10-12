/* Person.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Person.h"

#include "DataNode.h"
#include "GameData.h"
#include "Government.h"
#include "Ship.h"

#include <iostream>

using namespace std;



void Person::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "system")
			location.Load(child);
		if(child.Token(0) == "frequency" && child.Size() >= 2)
			frequency = child.Value(1);
		else if(child.Token(0) == "ship" && child.Size() >= 2)
		{
			ship.reset(new Ship);
			ship->Load(child);
		}
		else if(child.Token(0) == "government" && child.Size() >= 2)
			government = GameData::Governments().Get(child.Token(1));
		else if(child.Token(0) == "personality")
			personality.Load(child);
		else if(child.Token(0) == "phrase")
			hail.Load(child);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// Find out how often this person should appear in the given system. If this
// person is dead or already active, this will return zero.
int Person::Frequency(const System *system) const
{
	if(!system || !ship || IsDestroyed() || ship->GetSystem())
		return 0;
	
	return (location.IsEmpty() || location.Matches(system)) ? frequency : 0;
}



// Get the person's characteristics. The ship object is persistent, i.e. it
// will be recycled every time this person appears.
const shared_ptr<Ship> &Person::GetShip() const
{
	return ship;
}



const Government *Person::GetGovernment() const
{
	return government;
}



const Personality &Person::GetPersonality() const
{
	return personality;
}



const Phrase &Person::GetHail() const
{
	return hail;
}



bool Person::IsDestroyed() const
{
	return (ship->IsDestroyed() || (ship->GetSystem() && ship->GetGovernment() != government));
}
