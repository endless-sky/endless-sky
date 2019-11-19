/* Crew.cpp
Copyright (c) 2019 by Luke Arndt

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Crew.h"
#include "GameData.h"

using namespace std;

// Load definition of a crew member
void Crew::Load(const DataNode &node)
{
	// Set the id of this crew member so we know that we have loaded it.
	if(node.Size() >= 2)
		id = node.Token(1);
	
	for(const DataNode &child : node)
	{
		if(child.Size() >= 2)
		{
			if(child.Token(0) == "name")
				name = child.Token(1);
			// The number of credits paid daily (minimum 0)
			else if(child.Token(0) == "salary")
				salary = max((int)child.Value(1), 0);
			// Each valid ship has at least this many of the crew member
			else if(child.Token(0) == "minimum per ship")
				minimumPerShip = max((int)child.Value(1), 0);
			// Every nth crew member on the ship will be this crew member
			else if(child.Token(0) == "population per member")
				populationPerMember = max((int)child.Value(1), 0);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		// If true, the crew member will not appear on escorts
		else if(child.Token(0) == "avoids escorts")
			avoidsEscorts = true;
		// If true, the crew member will not appear on the flagship
		else if(child.Token(0) == "avoids flagship")
			avoidsFlagship = true;
		// If true, the crew member will receive salary even on a parked ship
		else if(child.Token(0) == "pay salary while parked")
			isPaidSalaryWhileParked = true;
		else
			child.PrintTrace("Skipping incomplete attribute:");
	}
}



int64_t Crew::CalculateSalaries(
	const vector<shared_ptr<Ship>> &ships,
	const bool includeExtras
)
{
	int64_t totalSalaries = 0;
	
	for(const shared_ptr<Ship> &ship : ships)
	{
		totalSalaries += Crew::SalariesForShip(
			ship,
			includeExtras
		);
	}
	
	return totalSalaries;
}



int64_t Crew::CostOfExtraCrew(
	const vector<shared_ptr<Ship>> &ships
)
{
	// Calculate with and without extras and return the difference.
	return Crew::CalculateSalaries(ships, true)
		- Crew::CalculateSalaries(ships, false);
}



int64_t Crew::NumberOnShip(
	const Crew &crew,
	const shared_ptr<Ship> &ship,
	const bool includeExtras
)
{
	int64_t count = 0;
	
	// If this is the flagship, check if this crew avoids the flagship.
	if(!ship->GetParent() && crew.AvoidsFlagship())
		return count;
	// If this is an escort, check if this crew avoids escorts.
	if(ship->GetParent() && crew.AvoidsEscorts())
		return count;
	
	const int64_t countableCrewMembers = includeExtras
		? ship->Crew()
		: ship->RequiredCrew();
	
	// Apply the per-ship minimum.
	 count = min(crew.MinimumPerShip(), countableCrewMembers);
	
	// Prevent division by zero so that the universe doesn't implode.
	if(crew.PopulationPerMember())
	{
		// Figure out how many of this kind of crew we have, by population.
		count = max(
			count,
			countableCrewMembers / crew.PopulationPerMember()
		);
	}
	
	return count;
}



int64_t Crew::SalariesForShip(
	const shared_ptr<Ship> &ship,
	const bool includeExtras
)
{
	// We don't need to pay dead people.
	if(ship->IsDestroyed())
		return 0;
	
	int64_t salariesForShip = 0;
	int64_t specialCrewMembers = 0;
	
	// Add up the salaries for all of the special crew members
	for(const pair<const string, Crew>& crewPair : GameData::Crews())
	{
		// Skip the default crew members.
		if(crewPair.first == "default")
			continue;
		
		const Crew crew = crewPair.second;
		// Figure out how many of this type of crew are on this ship
		int numberOnShip = Crew::NumberOnShip(
			crew,
			ship,
			includeExtras
		);
		
		specialCrewMembers += numberOnShip;
		
		// Add their salary to the pool
		// Unless the ship is parked and we don't pay them while parked
		if(crew.IsPaidSalaryWhileParked() || !ship->IsParked())
			salariesForShip += numberOnShip * crew.Salary();
	}
	
	// Figure out how many regular crew members are left over
	int64_t defaultCrewMembers = (
		includeExtras 
			? ship->Crew()
			: ship->RequiredCrew()
		) - specialCrewMembers
		// Subtract 1 if this is the flagship 
		- !ship->GetParent();

	const Crew *defaultCrew = GameData::Crews().Find("default");
	
	// Check if we pay salaries to parked default crew members
	if(defaultCrew->IsPaidSalaryWhileParked() || !ship->IsParked())
		// Add the default crew member salaries to the result
		salariesForShip += defaultCrewMembers * defaultCrew->Salary();
	
	return salariesForShip;
}



bool Crew::AvoidsEscorts() const
{
	return avoidsEscorts;
}



bool Crew::AvoidsFlagship() const
{
	return avoidsFlagship;
}



bool Crew::IsPaidSalaryWhileParked() const
{
	return isPaidSalaryWhileParked;
}



int64_t Crew::MinimumPerShip() const
{
	return minimumPerShip;
}



int64_t Crew::PopulationPerMember() const
{
	return populationPerMember;
}



int64_t Crew::Salary() const
{
	return salary;
}



const string &Crew::Id() const
{
	return id;
}



const string &Crew::Name() const
{
	return name;
}
