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
#include "CoreStartData.h"
#include "DataNode.h"
#include "Date.h"
#include "Depreciation.h"
#include "GameEvent.h"
#include "Mission.h"

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Government;
class Outfit;
class Planet;
class Rectangle;
class Ship;
class ShipEvent;
class StartConditions;
class StellarObject;
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
	void New(const StartConditions &start);
	// Load an existing player.
	void Load(const std::string &path);
	// Load the most recently saved player. If no save could be loaded, returns false.
	bool LoadRecent();
	// Save this player (using the Identifier() as the file name).
	void Save() const;
	
	// Get the root filename used for this player's saved game files. (If there
	// are multiple pilots with the same name it may have a digit appended.)
	std::string Identifier() const;
	
	// Apply the given changes and store them in the player's saved game file.
	void AddChanges(std::list<DataNode> &changes);
	// Add an event that will happen at the given date.
	void AddEvent(const GameEvent &event, const Date &date);
	
	// Mark the player as dead, or check if they have died.
	void Die(int response = 0, const std::shared_ptr<Ship> &capturer = nullptr);
	bool IsDead() const;
	
	// Get or set the player's name.
	const std::string &FirstName() const;
	const std::string &LastName() const;
	void SetName(const std::string &first, const std::string &last);
	
	// Get or change the current date.
	const Date &GetDate() const;
	void IncrementDate();
	
	// Get basic data about the player's starting scenario.
	const CoreStartData &StartData() const noexcept;
	
	// Set the system the player is in. This must be stored here so that even if
	// the player sells all their ships, we still know where the player is.
	// This also marks the given system as visited.
	void SetSystem(const System &system);
	const System *GetSystem() const;
	// Set what planet the player is on (or nullptr, if taking off).
	void SetPlanet(const Planet *planet);
	const Planet *GetPlanet() const;
	// If the player is landed, return the stellar object they are on.
	const StellarObject *GetStellarObject() const;
	// Check whether a mission conversation has raised a flag that the player
	// must leave the planet immediately (without time to do anything else).
	bool ShouldLaunch() const;
	
	// Access the player's accounting information.
	const Account &Accounts() const;
	Account &Accounts();
	// Calculate the daily salaries for crew, not counting crew on "parked" ships.
	int64_t Salaries() const;
	// Calculate the daily maintenance cost for all ships and in cargo outfits.
	int64_t Maintenance() const;
	
	// Access the flagship (the first ship in the list). This returns null if
	// the player does not have any ships that can be a flagship.
	const Ship *Flagship() const;
	Ship *Flagship();
	const std::shared_ptr<Ship> &FlagshipPtr();
	// Get the full list of ships the player owns.
	const std::vector<std::shared_ptr<Ship>> &Ships() const;
	// Inspect the flightworthiness of the player's active fleet as a whole to
	// determine which ships cannot travel with the group.
	std::map<const std::shared_ptr<Ship>, std::vector<std::string>> FlightCheck() const;
	// Add a captured ship to your fleet.
	void AddShip(const std::shared_ptr<Ship> &ship);
	// Buy or sell a ship.
	void BuyShip(const Ship *model, const std::string &name, bool isGift = false);
	void SellShip(const Ship *selected);
	void DisownShip(const Ship *selected);
	void ParkShip(const Ship *selected, bool isParked);
	void RenameShip(const Ship *selected, const std::string &name);
	// Change the order of the given ship in the list.
	void ReorderShip(int fromIndex, int toIndex);
	int ReorderShips(const std::set<int> &fromIndices, int toIndex);
	// Get the attraction factors of the player's fleet to raid fleets.
	std::pair<double, double> RaidFleetFactors() const;
	
	// Get cargo information.
	CargoHold &Cargo();
	const CargoHold &Cargo() const;
	// Get items stored on the player's current planet.
	CargoHold *Storage(bool forceCreate = false);
	// Get items stored on all planets (for map display).
	const std::map<const Planet *, CargoHold> &PlanetaryStorage() const;
	// Get cost basis for commodities.
	void AdjustBasis(const std::string &commodity, int64_t adjustment);
	int64_t GetBasis(const std::string &commodity, int tons = 1) const;
	// Call this when leaving the outfitter, shipyard, or hiring panel.
	void UpdateCargoCapacities();
	// Switch cargo from being stored in ships to being stored here.
	void Land(UI *ui);
	// Load the cargo back into your ships. This may require selling excess.
	bool TakeOff(UI *ui);

	// Get or add to pilot's playtime.
	double GetPlayTime() const noexcept;
	void AddPlayTime(std::chrono::nanoseconds timeVal);
	
	// Get the player's logbook.
	const std::multimap<Date, std::string> &Logbook() const;
	void AddLogEntry(const std::string &text);
	const std::map<std::string, std::map<std::string, std::string>> &SpecialLogs() const;
	void AddSpecialLog(const std::string &type, const std::string &name, const std::string &text);
	bool HasLogs() const;
	
	// Get mission information.
	const std::list<Mission> &Missions() const;
	const std::list<Mission> &AvailableJobs() const;
	const Mission *ActiveBoardingMission() const;
	void UpdateMissionNPCs();
	void AcceptJob(const Mission &mission, UI *ui);
	// Check to see if there is any mission to offer right now.
	Mission *MissionToOffer(Mission::Location location);
	Mission *BoardingMission(const std::shared_ptr<Ship> &ship);
	void ClearActiveBoardingMission();
	// If one of your missions cannot be offered because you do not have enough
	// space for it, and it specifies a message to be shown in that situation,
	// show that message.
	void HandleBlockedMissions(Mission::Location location, UI *ui);
	// Callback for accepting or declining whatever mission has been offered.
	void MissionCallback(int response);
	// Basic callback for handling forced departure from a planet.
	void BasicCallback(int response);
	// Complete or fail a mission.
	void RemoveMission(Mission::Trigger trigger, const Mission &mission, UI *ui);
	// Mark a mission as failed, but do not remove it from the mission list yet.
	void FailMission(const Mission &mission);
	// Update mission status based on an event.
	void HandleEvent(const ShipEvent &event, UI *ui);
	
	// Access the "condition" flags for this player.
	int64_t GetCondition(const std::string &name) const;
	std::map<std::string, int64_t> &Conditions();
	const std::map<std::string, int64_t> &Conditions() const;
	// Set and check the reputation conditions, which missions and events
	// can use to modify the player's reputation with other governments.
	void SetReputationConditions();
	void CheckReputationConditions();
	
	// Check what the player knows about the given system or planet.
	bool HasSeen(const System &system) const;
	bool HasVisited(const System &system) const;
	bool HasVisited(const Planet &planet) const;
	bool KnowsName(const System &system) const;
	// Marking a system as visited also "sees" its neighbors.
	void Visit(const System &system);
	void Visit(const Planet &planet);
	// Mark a system and its planets as unvisited, even if visited previously.
	void Unvisit(const System &system);
	void Unvisit(const Planet &planet);
	
	// Access the player's travel plan.
	bool HasTravelPlan() const;
	const std::vector<const System *> &TravelPlan() const;
	std::vector<const System *> &TravelPlan();
	// Remove the first or last system from the travel plan.
	void PopTravel();
	// Get or set the planet to land on at the end of the travel path.
	const Planet *TravelDestination() const;
	void SetTravelDestination(const Planet *planet);
	
	// Toggle which secondary weapon the player has selected.
	const Outfit *SelectedWeapon() const;
	void SelectNext();
	
	// Escorts currently selected for giving orders.
	const std::vector<std::weak_ptr<Ship>> &SelectedShips() const;
	// Select any player ships in the given box or list. Return true if any were
	// selected.
	bool SelectShips(const Rectangle &box, bool hasShift);
	bool SelectShips(const std::vector<const Ship *> &stack, bool hasShift);
	void SelectShip(const Ship *ship, bool hasShift);
	void SelectGroup(int group, bool hasShift);
	void SetGroup(int group, const std::set<Ship *> *newShips = nullptr);
	std::set<Ship *> GetGroup(int group);
	
	// Keep track of any outfits that you have sold since landing. These will be
	// available to buy back until you take off.
	int Stock(const Outfit *outfit) const;
	void AddStock(const Outfit *outfit, int count);
	// Get depreciation information.
	const Depreciation &FleetDepreciation() const;
	const Depreciation &StockDepreciation() const;
	
	// Keep track of what materials you have mined in each system.
	void Harvest(const Outfit *type);
	const std::set<std::pair<const System *, const Outfit *>> &Harvested() const;
	
	// Get or set the travel destination for selected escorts via the map.
	const std::pair<const System *, Point> &GetEscortDestination() const;
	void SetEscortDestination(const System *system = nullptr, Point pos = Point());
	bool HasEscortDestination() const;
	
	// Get or set what coloring is currently selected in the map.
	int MapColoring() const;
	void SetMapColoring(int index);
	// Get or set the map zoom level.
	int MapZoom() const;
	void SetMapZoom(int level);
	// Get the set of collapsed categories for the named panel.
	std::set<std::string> &Collapsed(const std::string &name);
	
	
private:
	// Don't allow anyone else to copy this class, because pointers won't get
	// transferred properly.
	PlayerInfo(const PlayerInfo &) = default;
	PlayerInfo &operator=(const PlayerInfo &) = default;
	
	// Apply any "changes" saved in this player info to the global game state.
	void ApplyChanges();
	// After loading & applying changes, make sure the player & ship locations are sensible.
	void ValidateLoad();
	
	// New missions are generated each time you land on a planet.
	void UpdateAutoConditions(bool isBoarding = false);
	void CreateMissions();
	void StepMissions(UI *ui);
	void Autosave() const;
	void Save(const std::string &path) const;
	
	// Check for and apply any punitive actions from planetary security.
	void Fine(UI *ui);
	
	// Helper function to update the ship selection.
	void SelectShip(const std::shared_ptr<Ship> &ship, bool *first);
	
	// Check that this player's current state can be saved.
	bool CanBeSaved() const;
	
	
private:
	std::string firstName;
	std::string lastName;
	std::string filePath;
	
	Date date;
	const System *system = nullptr;
	const Planet *planet = nullptr;
	bool shouldLaunch = false;
	bool isDead = false;
	
	// The amount of in-game time played, in seconds.
	double playTime = 0.0;
	
	Account accounts;
	
	std::shared_ptr<Ship> flagship;
	std::vector<std::shared_ptr<Ship>> ships;
	std::vector<std::weak_ptr<Ship>> selectedShips;
	std::map<const Ship *, int> groups;
	CargoHold cargo;
	std::map<const Planet *, CargoHold> planetaryStorage;
	std::map<std::string, int64_t> costBasis;
	
	std::multimap<Date, std::string> logbook;
	std::map<std::string, std::map<std::string, std::string>> specialLogs;
	
	// A list of the player's active, accepted missions.
	std::list<Mission> missions;
	// These lists are populated when you land on a planet, and saved so that
	// they will not change if you reload the game.
	std::list<Mission> availableJobs;
	std::list<Mission> availableMissions;
	// If any mission component is not fully defined, the mission is deactivated
	// until its components are fully evaluable (i.e. needed plugins are reinstalled).
	std::list<Mission> inactiveMissions;
	// Missions that are failed or aborted, but not yet deleted, and any
	// missions offered while in-flight are not saved.
	std::list<Mission> doneMissions;
	std::list<Mission> boardingMissions;
	// This pointer to the most recently accepted boarding mission enables
	// its NPCs to be placed before the player lands, and is then cleared.
	Mission *activeBoardingMission = nullptr;
	
	std::map<std::string, int64_t> conditions;
	
	std::set<const System *> seen;
	std::set<const System *> visitedSystems;
	std::set<const Planet *> visitedPlanets;
	std::vector<const System *> travelPlan;
	const Planet *travelDestination = nullptr;
	
	const Outfit *selectedWeapon = nullptr;
	
	std::map<const Outfit *, int> stock;
	Depreciation depreciation;
	Depreciation stockDepreciation;
	std::set<std::pair<const System *, const Outfit *>> harvested;
	
	// Changes that this PlayerInfo wants to make to the global galaxy state:
	std::vector<std::pair<const Government *, double>> reputationChanges;
	std::list<DataNode> dataChanges;
	DataNode economy;
	// Persons that have been killed in this player's universe:
	std::vector<std::string> destroyedPersons;
	// Events that are going to happen some time in the future:
	std::list<GameEvent> gameEvents;
	
	// The system and position therein to which the "orbits" system UI issued a move order.
	std::pair<const System *, Point> interstellarEscortDestination;
	// Currently selected coloring, in the map panel (defaults to reputation):
	int mapColoring = -6;
	int mapZoom = 0;
	
	// Currently collapsed categories for various panels.
	std::map<std::string, std::set<std::string>> collapsed;
	
	bool freshlyLoaded = true;
	int desiredCrew = 0;
	
	// Basic information about the player's starting scenario.
	CoreStartData startData;
};



#endif
