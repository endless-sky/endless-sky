/* PlayerInfo.cpp
Michael Zahniser, 1 Jan 2013

Function definitions for the PlayerInfo class.
*/

#include "PlayerInfo.h"

#include "Outfit.h"
#include "Ship.h"
#include "System.h"

using namespace std;



PlayerInfo::PlayerInfo()
	: selectedWeapon(nullptr)
{
}



const Account &PlayerInfo::Accounts() const
{
	return accounts;
}



Account &PlayerInfo::Accounts()
{
	return accounts;
}



// Set the player ship.
void PlayerInfo::AddShip(Ship *ship)
{
	ships.push_back(ship);
}



const Ship *PlayerInfo::GetShip() const
{
	return ships.empty() ? nullptr : ships.front();
}



Ship *PlayerInfo::GetShip()
{
	return ships.empty() ? nullptr : ships.front();
}



const vector<Ship *> &PlayerInfo::Ships() const
{
	return ships;
}



bool PlayerInfo::HasSeen(const System *system) const
{
	return (seen.find(system) != seen.end());
}



bool PlayerInfo::HasVisited(const System *system) const
{
	return (visited.find(system) != visited.end());
}



void PlayerInfo::Visit(const System *system)
{
	visited.insert(system);
	seen.insert(system);
	for(const System *neighbor : system->Neighbors())
		seen.insert(neighbor);
}



bool PlayerInfo::HasTravelPlan() const
{
	return !travelPlan.empty();
}



const vector<const System *> &PlayerInfo::TravelPlan() const
{
	return travelPlan;
}



void PlayerInfo::ClearTravel()
{
	travelPlan.clear();
}



// Add to the travel plan, starting with the last system in the journey.
void PlayerInfo::AddTravel(const System *system)
{
	travelPlan.push_back(system);
}



void PlayerInfo::PopTravel()
{
	if(!travelPlan.empty())
	{
		Visit(travelPlan.back());
		travelPlan.pop_back();
	}
}



// Toggle which secondary weapon the player has selected.
const Outfit *PlayerInfo::SelectedWeapon() const
{
	return selectedWeapon;
}



void PlayerInfo::SelectNext()
{
	if(ships.empty())
		return;
	
	const Ship *ship = ships.front();
	if(ship->Outfits().empty())
		return;
	
	auto it = ship->Outfits().find(selectedWeapon);
	if(it == ship->Outfits().end())
		it = ship->Outfits().begin();
	
	while(++it != ship->Outfits().end())
		if(it->first->Ammo())
		{
			selectedWeapon = it->first;
			return;
		}
	selectedWeapon = nullptr;
}
