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
#include "GameEvent.h"
#include "Mission.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <string>

class DataNode;
class Government;
class Outfit;
class Planet;
class Ship;
class ShipEvent;
class System;
class UI;



// Class holding information about a "player" - their name, their finances, what
// ship(s) they own and with what outfits, what systems they have visited, etc.
// This class also handles saving the player's info to disk so it can be read
// back in exactly the same state later. This includes what changes the player
// has made to the universe, what jobs are being offered to them right now,
// and what their current travel plan is, if any.
class PlayerInfo {
public:
	PlayerInfo();
	
	void Clear();
	// Don't allow copying, because pointers won't get transferred properly.
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
	void AddChanges(std::list<DataNode> &changes);
	// Add an event that will happen at the given date.
	void AddEvent(const GameEvent &event, const Date &date);
	
	// Mark the player as dead, or check if they have died.
	void Die();
	bool IsDead() const;
	
	const std::string &FirstName() const;
	const std::string &LastName() const;
	void SetName(const std::string &first, const std::string &last);
	
	const Date &GetDate() const;
	void IncrementDate();
	
	// Set the system the player is in. This must be stored here so that even if
	// the player sells all their ships, we still know where the player is.
	// This also marks the given system as visited.
	void SetSystem(const System *system);
	const System *GetSystem() const;
	void SetPlanet(const Planet *planet);
	const Planet *GetPlanet() const;
	bool ShouldLaunch() const;
	
	const Account &Accounts() const;
	Account &Accounts();
	int64_t Salaries() const;
	
	const Ship *Flagship() const;
	Ship *Flagship();
	const std::vector<std::shared_ptr<Ship>> &Ships() const;
	// Add a captured ship to your fleet.
	void AddShip(std::shared_ptr<Ship> &ship);
	void RemoveShip(const std::shared_ptr<Ship> &ship);
	// Buy or sell a ship.
	void BuyShip(const Ship *model, const std::string &name);
	void SellShip(const Ship *selected);
	void ParkShip(const Ship *selected, bool isParked);
	void RenameShip(const Ship *selected, const std::string &name);
	// Change the order of the given ship in the list.
	void ReorderShip(int fromIndex, int toIndex);
	
	// Get cargo information.
	CargoHold &Cargo();
	const CargoHold &Cargo() const;
	// Call this when leaving the outfitter, shipyard, or hiring panel.
	void UpdateCargoCapacities();
	// Switch cargo from being stored in ships to being stored here.
	void Land(UI *ui);
	// Load the cargo back into your ships. This may require selling excess.
	void TakeOff();
	
	// Get mission information.
	const std::list<Mission> &Missions() const;
	const std::list<Mission> &AvailableJobs() const;
	void AcceptJob(const Mission &mission);
	// Check to see if there is any mission to offer in the spaceport right now.
	Mission *MissionToOffer(Mission::Location location);
	// Callback for accepting or declining whatever mission has been offered.
	void MissionCallback(int response);
	// Complete or fail a mission.
	void RemoveMission(Mission::Trigger trigger, const Mission &mission, UI *ui);
	// Update mission status based on an event.
	void HandleEvent(const ShipEvent &event, UI *ui);
	
	int GetCondition(const std::string &name) const;
	std::map<std::string, int> &Conditions();
	const std::map<std::string, int> &Conditions() const;
	
	bool HasSeen(const System *system) const;
	bool HasVisited(const System *system) const;
	bool KnowsName(const System *system) const;
	void Visit(const System *system);
	// Mark a system as unvisited, even if visited previously.
	void Unvisit(const System *system);
	
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
	
	// Keep track of any outfits that you have sold since landing. These will be
	// available to buy back until you take off.
	std::map<const Outfit *, int> &SoldOutfits();
	
	
private:
	// New missions are generated each time you land on a planet.
	void CreateMissions();
	
	
private:
	std::string firstName;
	std::string lastName;
	std::string filePath;
	
	Date date;
	const System *system;
	const Planet *planet;
	bool shouldLaunch;
	bool hasFullClearance;
	bool isDead;
	
	Account accounts;
	
	std::vector<std::shared_ptr<Ship>> ships;
	CargoHold cargo;
	
	std::list<Mission> missions;
	// These lists are populated when you land on a planet, and saved so that
	// they will not change if you reload the game.
	std::list<Mission> availableJobs;
	std::list<Mission> availableMissions;
	std::list<Mission> doneMissions;
	
	std::map<std::string, int> conditions;
	
	std::set<const System *> seen;
	std::set<const System *> visited;
	std::vector<const System *> travelPlan;
	
	const Outfit *selectedWeapon;
	
	std::map<const Outfit *, int> soldOutfits;
	
	// Changes that this PlayerInfo wants to make to the global galaxy state:
	std::vector<std::pair<const Government *, double>> reputationChanges;
	std::list<DataNode> dataChanges;
	// Events that are going to happen some time in the future:
	std::list<GameEvent> gameEvents;
	
	bool freshlyLoaded;
};



#endif
