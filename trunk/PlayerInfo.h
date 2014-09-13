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
#include "CargoHold.h"
#include "Date.h"
#include "Mission.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <string>

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
	// Don't allow copying, because the mission pointers won't get transferred properly.
	void Steal(PlayerInfo &other);
	PlayerInfo(const PlayerInfo &) = delete;
	PlayerInfo &operator=(const PlayerInfo &) = delete;
	
	bool IsLoaded() const;
	void Load(const std::string &path);
	void Save() const;
	std::string Identifier() const;
	
	// Load the most recently saved player.
	void LoadRecent();
	// Make a new player.
	void New();
	// Apply any "changes" saved in this player info to the global game state.
	void ApplyChanges();
	
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
	int64_t Salaries() const;
	
	// Set the player ship.
	void AddShip(std::shared_ptr<Ship> &ship);
	void RemoveShip(const std::shared_ptr<Ship> &ship);
	const Ship *GetShip() const;
	Ship *GetShip();
	const std::vector<std::shared_ptr<Ship>> &Ships() const;
	// Buy or sell a ship.
	void BuyShip(const Ship *model, const std::string &name);
	void SellShip(const Ship *selected);
	// Change the order of the given ship in the list.
	void ReorderShip(int fromIndex, int toIndex);
	
	// Get cargo information.
	CargoHold &Cargo();
	const CargoHold &Cargo() const;
	// Call this when leaving the oufitter, shipyard, or hiring panel.
	void UpdateCargoCapacities();
	// Switch cargo from being stored in ships to being stored here.
	void Land();
	// Load the cargo back into your ships. This may require selling excess.
	void TakeOff();
	
	// Get mission information.
	const std::list<Mission> &Missions() const;
	const std::list<Mission> &AvailableJobs() const;
	// Check if your fleet has space for the given mission.
	bool CanAccept(const Mission &mission) const;
	void AcceptJob(const Mission &mission);
	void AddMission(const Mission &mission);
	void AbortMission(const Mission &mission);
	void CompleteMission(const Mission &mission);
	// Callback for accepting or declining whatever mission has been offered.
	void MissionCallback(int response);
	
	// Special missions.
	const Mission *NextSpecialMission() const;
	void AcceptSpecialMission();
	void DeclineSpecialMission();
	const std::list<const Mission *> &SpecialMissions() const;
	
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
	// New missions are generated each time you land on a planet. This also means
	// that random missions that didn't show up might show up if you reload the
	// game, but that's a minor detail and I can fix it later.
	void CreateMissions();
	
	
private:
	std::string firstName;
	std::string lastName;
	std::string filePath;
	
	Date date;
	const System *system;
	const Planet *planet;
	Account accounts;
	
	std::vector<std::shared_ptr<Ship>> ships;
	CargoHold cargo;
	std::list<Mission> missions;
	// This list is populated when you are landed on a planet. It must be saved
	// in order to avoid presenting duplicate missions when loading again.
	std::list<Mission> jobs;
	
	std::list<const Mission *> availableSpecials;
	std::list<const Mission *> specials;
	
	std::map<std::string, int> conditions;
	
	std::set<const System *> seen;
	std::set<const System *> visited;
	std::vector<const System *> travelPlan;
	
	const Outfit *selectedWeapon;
	
	// Changes that this PlayerInfo wants to make to the global galaxy state:
	std::vector<std::pair<const Government *, double>> reputationChanges;
	
	bool freshlyLoaded;
};



#endif
