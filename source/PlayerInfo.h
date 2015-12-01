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
class Person;
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
	PlayerInfo() = default;
	
	// Reset the player to an "empty" state, i.e. no player is loaded.
	void Clear();
	
	// Check if any player's information is loaded.
	bool IsLoaded() const;
	// Make a new player.
	void New();
	// Load an existing player.
	void Load(const std::string &path);
	// Load the most recently saved player.
	void LoadRecent();
	// Save this player (using the Identifier() as the file name).
	void Save() const;
	
	// Get the root filename used for this player's saved game files. (If there
	// are multiple pilots with the same name it may have a digit appended.)
	std::string Identifier() const;
	
	// Apply any "changes" saved in this player info to the global game state.
	void ApplyChanges();
	// Apply the given changes and store them in the player's saved game file.
	void AddChanges(std::list<DataNode> &changes);
	// Add an event that will happen at the given date.
	void AddEvent(const GameEvent &event, const Date &date);
	
	// Mark the player as dead, or check if they have died.
	void Die(bool allShipsDie = false);
	bool IsDead() const;
	
	// Get or set the player's name.
	const std::string &FirstName() const;
	const std::string &LastName() const;
	void SetName(const std::string &first, const std::string &last);
	
	// Get or change the current date.
	const Date &GetDate() const;
	void IncrementDate();
	
	// Set the system the player is in. This must be stored here so that even if
	// the player sells all their ships, we still know where the player is.
	// This also marks the given system as visited.
	void SetSystem(const System *system);
	const System *GetSystem() const;
	// Set what planet the player is on.
	void SetPlanet(const Planet *planet);
	const Planet *GetPlanet() const;
	// Check whether a mission conversation has raised a flag that the player
	// must leave the planet immediately (without time to do anything else).
	bool ShouldLaunch() const;
	
	// Access the player's accountign information.
	const Account &Accounts() const;
	Account &Accounts();
	// Calculate the daily salaries for crew, not counting crew on "parked" ships.
	int64_t Salaries() const;
	
	// Access the flagship (the first ship in the list). This returns null if
	// the player does not have any ships.
	const Ship *Flagship() const;
	Ship *Flagship();
	const std::shared_ptr<Ship> &FlagshipPtr();
	// Get the full list of ships the player owns.
	const std::vector<std::shared_ptr<Ship>> &Ships() const;
	// Add a captured ship to your fleet.
	void AddShip(std::shared_ptr<Ship> &ship);
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
	// Get cost basis for commodities.
	void AdjustBasis(const std::string &commodity, int64_t adjustment);
	int64_t GetBasis(const std::string &commodity, int tons = 1) const;
	// Call this when leaving the outfitter, shipyard, or hiring panel.
	void UpdateCargoCapacities();
	// Switch cargo from being stored in ships to being stored here.
	void Land(UI *ui);
	// Load the cargo back into your ships. This may require selling excess.
	void TakeOff(UI *ui);
	
	// Get mission information.
	const std::list<Mission> &Missions() const;
	const std::list<Mission> &AvailableJobs() const;
	void AcceptJob(const Mission &mission);
	// Check to see if there is any mission to offer in the spaceport right now.
	Mission *MissionToOffer(Mission::Location location);
	Mission *BoardingMission(const std::shared_ptr<Ship> &ship);
	const std::shared_ptr<Ship> &BoardingShip() const;
	// If one of your missions cannot be offered because you do not have enough
	// space for it, and it specifies a message to be shown in that situation,
	// show that message.
	void HandleBlockedMissions(Mission::Location location, UI *ui);
	// Callback for accepting or declining whatever mission has been offered.
	void MissionCallback(int response);
	// Complete or fail a mission.
	void RemoveMission(Mission::Trigger trigger, const Mission &mission, UI *ui);
	// Update mission status based on an event.
	void HandleEvent(const ShipEvent &event, UI *ui);
	
	// Access the "condition" flags for this player.
	int GetCondition(const std::string &name) const;
	std::map<std::string, int> &Conditions();
	const std::map<std::string, int> &Conditions() const;
	
	// Check what the player knows about the given system.
	bool HasSeen(const System *system) const;
	bool HasVisited(const System *system) const;
	bool KnowsName(const System *system) const;
	void Visit(const System *system);
	// Mark a system as unvisited, even if visited previously.
	void Unvisit(const System *system);
	
	// Access the player's travel plan.
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
	// Don't anyone else to copy this class, because pointers won't get
	// transferred properly.
	PlayerInfo(const PlayerInfo &) = default;
	PlayerInfo &operator=(const PlayerInfo &) = default;
	
	// New missions are generated each time you land on a planet.
	void UpdateAutoConditions();
	void CreateMissions();
	void Autosave() const;
	void Save(const std::string &path) const;
	
	
private:
	std::string firstName;
	std::string lastName;
	std::string filePath;
	
	Date date;
	const System *system = nullptr;
	const Planet *planet = nullptr;
	bool shouldLaunch = false;
	bool hasFullClearance = true;
	bool isDead = false;
	
	Account accounts;
	
	std::shared_ptr<Ship> flagship;
	std::vector<std::shared_ptr<Ship>> ships;
	CargoHold cargo;
	std::map<std::string, int64_t> costBasis;
	
	std::list<Mission> missions;
	// These lists are populated when you land on a planet, and saved so that
	// they will not change if you reload the game.
	std::list<Mission> availableJobs;
	std::list<Mission> availableMissions;
	std::list<Mission> boardingMissions;
	std::shared_ptr<Ship> boardingShip;
	std::list<Mission> doneMissions;
	
	std::map<std::string, int> conditions;
	
	std::set<const System *> seen;
	std::set<const System *> visited;
	std::vector<const System *> travelPlan;
	
	const Outfit *selectedWeapon = nullptr;
	
	std::map<const Outfit *, int> soldOutfits;
	
	// Changes that this PlayerInfo wants to make to the global galaxy state:
	std::vector<std::pair<const Government *, double>> reputationChanges;
	std::list<DataNode> dataChanges;
	// Persons that have been killed in this player's universe:
	std::list<const Person *> destroyedPersons;
	// Events that are going to happen some time in the future:
	std::list<GameEvent> gameEvents;
	
	bool freshlyLoaded = true;
};



#endif
