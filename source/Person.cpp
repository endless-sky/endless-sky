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
#include "System.h"

using namespace std;



void Person::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "system")
			location.Load(child);
		else if(child.Token(0) == "frequency" && child.Size() >= 2)
			frequency = child.Value(1);
		else if(child.Token(0) == "ship" && child.Size() >= 2)
		{
			// Name ships that are not the flagship with the name provided, if any.
			// The flagship, and any unnamed fleet members, will be given the name of the Person.
			bool setName = !ships.empty() && child.Size() >= 3;
			ships.emplace_back(new Ship(child));
			if(setName)
				ships.back()->SetName(child.Token(2));
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



// Finish loading all the ships in this person specification.
void Person::FinishLoading()
{
	for(const shared_ptr<Ship> &ship : ships)
		ship->FinishLoading(true);
}



// Find out how often this person should appear in the given system. If this
// person is dead or already active, this will return zero.
int Person::Frequency(const System *system) const
{
	// Because persons always enter a system via one of the regular hyperspace
	// links, don't create them in systems with no links.
	if(!system || IsDestroyed() || IsPlaced() || system->Links().empty())
		return 0;
	
	return (location.IsEmpty() || location.Matches(system)) ? frequency : 0;
}



// Get the person's characteristics. The ship object is persistent, i.e. it
// will be recycled every time this person appears.
const list<shared_ptr<Ship>> &Person::Ships() const
{
	return ships;
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
	if(ships.empty() || !ships.front())
		return true;
	
	const Ship &flagship = *ships.front();
	return (flagship.IsDestroyed() || (flagship.GetSystem() && flagship.GetGovernment() != government));
}



// Mark this person as destroyed.
void Person::Destroy()
{
	for(const shared_ptr<Ship> &ship : ships)
		ship->Destroy();
}



// Mark this person as no longer destroyed.
void Person::Restore()
{
	for(const shared_ptr<Ship> &ship : ships)
	{
		ship->Restore();
		ship->SetSystem(nullptr);
		ship->SetPlanet(nullptr);
	}
}



// Check if a person is already placed somehwere.
bool Person::IsPlaced() const
{
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->GetSystem())
			return true;
	
	return false;
}



// Mark this person as being no longer "placed" somewhere.
void Person::ClearPlacement()
{
	if(!IsDestroyed())
		Restore();
}
