/* PlayerInfo.h
Michael Zahniser, 1 Jan 2013

Class holding information about a "player" - their name, their finances, what
ship(s) they own and with what outfits, what systems they have visited, etc.
*/

#ifndef PLAYER_INFO_H_INCLUDED
#define PLAYER_INFO_H_INCLUDED

#include "Account.h"
#include "Date.h"

#include <memory>
#include <set>
#include <vector>
#include <string>

class GameData;
class Government;
class Outfit;
class Planet;
class Ship;
class System;



class PlayerInfo {
public:
	PlayerInfo();
	
	void Clear();
	bool IsLoaded() const;
	void Load(const std::string &path, const GameData &data);
	void Save() const;
	
	// Load the most recently saved player.
	void LoadRecent(const GameData &data);
	// Make a new player.
	void New(const GameData &data);
	
	const std::string &FirstName() const;
	const std::string &LastName() const;
	void SetName(const std::string &first, const std::string &last);
	
	const Date &GetDate() const;
	// Increment the date, and return a string summarizing daily payments.
	std::string IncrementDate();
	
	// Set the system the player is in. This must be stored here so that even if
	// the player sells all their ships, we still know where the player is.
	// This also marks the given system as visited.
	void SetSystem(const System *system);
	const System *GetSystem() const;
	void SetPlanet(const Planet *planet);
	const Planet *GetPlanet() const;
	
	const Account &Accounts() const;
	Account &Accounts();
	
	// Set the player ship.
	void AddShip(std::shared_ptr<Ship> &ship);
	const Ship *GetShip() const;
	Ship *GetShip();
	const std::vector<std::shared_ptr<Ship>> &Ships() const;
	// Buy or sell a ship.
	void BuyShip(const Ship *model);
	void SellShip(const Ship *selected);
	
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
	std::string firstName;
	std::string lastName;
	std::string filePath;
	
	Date date;
	const System *system;
	const Planet *planet;
	const Government *playerGovernment;
	Account accounts;
	
	std::vector<std::shared_ptr<Ship>> ships;
	std::set<const System *> seen;
	std::set<const System *> visited;
	std::vector<const System *> travelPlan;
	
	const Outfit *selectedWeapon;
};



#endif
