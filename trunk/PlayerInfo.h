/* PlayerInfo.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PLAYER_INFO_H_
#define PLAYER_INFO_H_

#include "Account.h"
#include "Date.h"

#include <map>
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



// Class holding information about a "player" - their name, their finances, what
// ship(s) they own and with what outfits, what systems they have visited, etc.
class PlayerInfo {
public:
	PlayerInfo();
	
	void Clear();
	bool IsLoaded() const;
	void Load(const std::string &path, const GameData &data);
	void Save() const;
	std::string Identifier() const;
	
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
	void BuyShip(const Ship *model, const std::string &name);
	void SellShip(const Ship *selected);
	
	// Get the cargo capacity of all in-system ships. This works whether or not
	// you have unloaded the cargo.
	int FreeCargo() const;
	// Switch cargo from being stored in ships to being stored here.
	void Land();
	// Load the cargo back into your ships. This may require selling excess, in
	// which case a message will be returned.
	std::string TakeOff();
	// Normal cargo and spare parts:
	std::map<std::string, int> Cargo() const;
	int Cargo(const std::string &type) const;
	std::map<const Outfit *, int> OutfitCargo() const;
	// Buy or sell cargo.
	void BuyCargo(const std::string &type, int amount);
	void AddOutfitCargo(const Outfit *outfit, int amount);
	
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
	const GameData *gameData;
	
	std::string firstName;
	std::string lastName;
	std::string filePath;
	
	Date date;
	const System *system;
	const Planet *planet;
	const Government *playerGovernment;
	Account accounts;
	
	std::vector<std::shared_ptr<Ship>> ships;
	std::map<std::string, int> cargo;
	std::map<const Outfit *, int> outfitCargo;
	
	std::set<const System *> seen;
	std::set<const System *> visited;
	std::vector<const System *> travelPlan;
	
	const Outfit *selectedWeapon;
};



#endif
