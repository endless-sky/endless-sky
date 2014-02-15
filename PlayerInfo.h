/* PlayerInfo.h
Michael Zahniser, 1 Jan 2013

Class holding information about a "player" - their name, their finances, what
ship(s) they own and with what outfits, what systems they have visited, etc.
*/

#ifndef PLAYER_INFO_H_INCLUDED
#define PLAYER_INFO_H_INCLUDED

#include "Account.h"

#include <memory>
#include <set>
#include <vector>

class Outfit;
class Ship;
class System;



class PlayerInfo {
public:
	PlayerInfo();
	
	const Account &Accounts() const;
	Account &Accounts();
	
	// Set the player ship.
	void AddShip(std::shared_ptr<Ship> &ship);
	const Ship *GetShip() const;
	Ship *GetShip();
	const std::vector<std::shared_ptr<Ship>> &Ships() const;
	
	bool HasSeen(const System *system) const;
	bool HasVisited(const System *system) const;
	
	void Visit(const System *system);
	
	bool HasTravelPlan() const;
	const std::vector<const System *> &TravelPlan() const;
	void ClearTravel();
	// Add to the travel plan, starting with the last system in the journey.
	void AddTravel(const System *system);
	// Remove the first system from the travel plan.
	void PopTravel();
	
	// Toggle which secondary weapon the player has selected.
	const Outfit *SelectedWeapon() const;
	void SelectNext();
	
	
private:
	Account accounts;
	
	std::vector<std::shared_ptr<Ship>> ships;
	std::set<const System *> seen;
	std::set<const System *> visited;
	std::vector<const System *> travelPlan;
	
	const Outfit *selectedWeapon;
};



#endif
