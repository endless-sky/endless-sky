/* PlayerInfo.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Account.h"
#include "CargoHold.h"
#include "ConditionsStore.h"
#include "CoreStartData.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Date.h"
#include "Depreciation.h"
#include "EsUuid.h"
#include "ExclusiveItem.h"
#include "GameEvent.h"
#include "Mission.h"
#include "SystemEntry.h"

#include <chrono>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

class DistanceMap;
class Outfit;
class Planet;
class RaidFleet;
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
	struct FleetBalance {
		int64_t maintenanceCosts = 0;
		int64_t assetsReturns = 0;
	};
	enum SortType {
		ABC,
		PAY,
		SPEED,
		CONVENIENT
	};


public:
	PlayerInfo() = default;
	// Don't allow copying this class.
	PlayerInfo(const PlayerInfo &) = delete;
	PlayerInfo &operator=(const PlayerInfo &) = delete;
	PlayerInfo(PlayerInfo &&) = delete;
	PlayerInfo &operator=(PlayerInfo &&) = default;
	~PlayerInfo() noexcept = default;

	// Reset the player to an "empty" state, i.e. no player is loaded.
	void Clear();

	// Check if any player's information is loaded.
	bool IsLoaded() const;
	// Make a new player.
	void New(const StartConditions &start);
	// Load an existing player.
	void Load(const std::filesystem::path &path);
	// Load the most recently saved player. If no save could be loaded, returns false.
	bool LoadRecent();
	// Save this player (using the Identifier() as the file name).
	void Save() const;

	// Get the root filename used for this player's saved game files. (If there
	// are multiple pilots with the same name it may have a digit appended.)
	std::string Identifier() const;

	// Start a transaction. This stores the current state, and any Save()
	// calls during the transaction will store this saved state.
	void StartTransaction();
	// Complete the transaction.
	void FinishTransaction();

	// Apply the given changes and store them in the player's saved game file.
	void AddChanges(std::list<DataNode> &changes, bool instantChanges = false);
	// Add an event that will happen at the given date.
	void AddEvent(GameEvent event, const Date &date);

	// Mark the player as dead, or check if they have died.
	void Die(int response = 0, const std::shared_ptr<Ship> &capturer = nullptr);
	bool IsDead() const;

	// Get or set the player's name.
	const std::string &FirstName() const;
	const std::string &LastName() const;
	void SetName(const std::string &first, const std::string &last);

	// Get or change the current date.
	const Date &GetDate() const;
	void AdvanceDate(int amount = 1);

	// Get basic data about the player's starting scenario.
	const CoreStartData &StartData() const noexcept;

	// Sets the means the player used to enter the system.
	void SetSystemEntry(SystemEntry entryType);
	SystemEntry GetSystemEntry() const;
	// Set the system the player is in. This must be stored here so that even if
	// the player sells all their ships, we still know where the player is.
	// This also marks the given system as visited.
	void SetSystem(const System &system);
	const System *GetSystem() const;
	const System *GetPreviousSystem() const;
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
	// Calculate the daily maintenance cost and generated income for all ships and in cargo outfits.
	FleetBalance MaintenanceAndReturns() const;

	// Access to the licenses the player owns.
	void AddLicense(const std::string &name);
	void RemoveLicense(const std::string &name);
	bool HasLicense(const std::string &name) const;
	const std::set<std::string> &Licenses() const;

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
	// Buy, receive or sell a ship.
	// In the case of a gift, return a pointer to the newly instantiated ship.
	void BuyShip(const Ship *model, const std::string &name);
	const Ship *GiftShip(const Ship *model, const std::string &name, const std::string &id);
	void SellShip(const Ship *selected, bool storeOutfits = false);
	// Take the ship from the player, if a model is specified this will permanently remove outfits in said model,
	// instead of allowing the player to buy them back by putting them in the stock.
	void TakeShip(const Ship *shipToTake, const Ship *model = nullptr, bool takeOutfits = false);
	std::vector<std::shared_ptr<Ship>>::iterator DisownShip(const Ship *selected);
	void ParkShip(const Ship *selected, bool isParked);
	void RenameShip(const Ship *selected, const std::string &name);
	// Change the order of the given ship in the list.
	void ReorderShip(int fromIndex, int toIndex);
	void SetShipOrder(const std::vector<std::shared_ptr<Ship>> &newOrder);
	// Get the attraction factors of the player's fleet to raid fleets.
	std::pair<double, double> RaidFleetFactors() const;
	double RaidFleetAttraction(const RaidFleet &raidFleet, const System *system) const;

	// Get cargo information.
	CargoHold &Cargo();
	const CargoHold &Cargo() const;
	// Get items stored on the player's current planet.
	CargoHold &Storage();
	// Get items stored on all planets (for map display).
	const std::map<const Planet *, CargoHold> &PlanetaryStorage() const;
	// Get cost basis for commodities.
	void AdjustBasis(const std::string &commodity, int64_t adjustment);
	int64_t GetBasis(const std::string &commodity, int tons = 1) const;
	// Call this when leaving the outfitter, shipyard, or hiring panel.
	void UpdateCargoCapacities();
	// Switch cargo from being stored in ships to being stored here.
	void Land(UI *ui);
	// Make ships ready for take off. This may require selling excess cargo.
	bool TakeOff(UI *ui, bool distributeCargo);
	// Pool cargo from local ships.
	void PoolCargo();
	// Distribute cargo to local ships. Returns a reference to the player's cargo.
	const CargoHold &DistributeCargo();

	// Get or add to pilot's playtime.
	double GetPlayTime() const noexcept;
	void AddPlayTime(std::chrono::nanoseconds timeVal);

	// Get the player's logbook.
	const std::map<Date, BookEntry> &Logbook() const;
	void AddLogEntry(const BookEntry &logbookEntry);
	const std::map<std::string, std::map<std::string, BookEntry>> &SpecialLogs() const;
	void AddSpecialLog(const std::string &category, const std::string &heading, const BookEntry &logbookEntry);
	void RemoveSpecialLog(const std::string &category, const std::string &heading);
	void RemoveSpecialLog(const std::string &category);
	bool HasLogs() const;

	// Get mission information.
	const std::list<Mission> &Missions() const;
	const std::list<Mission> &AvailableJobs() const;
	bool HasAvailableEnteringMissions() const;

	// For all active missions, cache information that can be requested often but does not change often,
	// or needs to be calculated at least once.
	// - Determine how many days left the player has for each mission with a deadline, for
	// the purpose of determining how frequently the MapPanel should blink the mission
	// marker.
	// - Determine which systems any tracked NPCs are located in.
	void CacheMissionInformation(bool onlyDeadlines = false);
	// Cache information for an individual mission, such as one that was just accepted.
	void CacheMissionInformation(Mission &mission, const DistanceMap &here, bool onlyDeadlines = false);
	// The number of days left before this mission's deadline has elapsed, or,
	// if the "Deadline blink by distance" preference is true, before the player
	// doesn't have enough days left to complete the mission before the deadline
	// will elapse. Returns 0 if the give mission doesn't have a deadline.
	int RemainingDeadline(const Mission &mission) const;

	const SortType GetAvailableSortType() const;
	void NextAvailableSortType();
	const bool ShouldSortAscending() const;
	void ToggleSortAscending();
	const bool ShouldSortSeparateDeadline() const;
	void ToggleSortSeparateDeadline();
	const bool ShouldSortSeparatePossible() const;
	void ToggleSortSeparatePossible();
	void SortAvailable();

	const Mission *ActiveInFlightMission() const;
	void UpdateMissionNPCs();
	void AcceptJob(const Mission &mission, UI *ui);
	// Check to see if there is any mission to offer right now.
	Mission *MissionToOffer(Mission::Location location);
	Mission *BoardingMission(const std::shared_ptr<Ship> &ship);
	void CreateEnteringMissions();
	Mission *EnteringMission();
	// Return true if the given ship is capturable only because it's the source
	// of a boarding mission which allows it to be.
	bool CaptureOverriden(const std::shared_ptr<Ship> &ship) const;
	void ClearActiveInFlightMission();
	// If one of your missions cannot be offered because you do not have enough
	// space for it, and it specifies a message to be shown in that situation,
	// show that message.
	void HandleBlockedMissions(Mission::Location location, UI *ui);
	// Display the blocked message for the first available entering mission,
	// then remove it from the available entering missions list.
	void HandleBlockedEnteringMissions(UI *ui);
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
	ConditionsStore &Conditions();
	const ConditionsStore &Conditions() const;
	// Maps defined names for gifted ships to UUIDs for the ship instances.
	const std::map<std::string, EsUuid> &GiftedShips() const;
	std::map<std::string, std::string> GetSubstitutions() const;
	void AddPlayerSubstitutions(std::map<std::string, std::string> &subs) const;

	// Get and set the "tribute" that the player receives from dominated planets.
	bool SetTribute(const Planet *planet, int64_t payment);
	bool SetTribute(const std::string &planetTrueName, int64_t payment);
	const std::map<const Planet *, int64_t> &GetTribute() const;
	int64_t GetTributeTotal() const;

	// Check what the player knows about the given system or planet.
	bool HasSeen(const System &system) const;
	bool CanView(const System &system) const;
	bool HasVisited(const System &system) const;
	bool HasVisited(const Planet &planet) const;
	bool KnowsName(const System &system) const;
	// Marking a system as visited also "sees" its neighbors.
	void Visit(const System &system);
	void Visit(const Planet &planet);
	// Mark a system and its planets as unvisited, even if visited previously.
	void Unvisit(const System &system);
	void Unvisit(const Planet &planet);
	const std::set<const System *> &VisitedSystems() const;
	const std::set<const Planet *> &VisitedPlanets() const;

	// Check whether the player has visited the <mapSize> systems around the current one.
	bool HasMapped(int mapSize, bool mapMinables) const;
	// Mark a whole map of systems as visited.
	void Map(int mapSize, bool mapMinables);

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
	const std::set<const Outfit *> &SelectedSecondaryWeapons() const;
	void SelectNextSecondary();
	void DeselectAllSecondaries();
	void ToggleAnySecondary(const Outfit *outfit);

	// Escorts currently selected for giving orders.
	const std::vector<std::weak_ptr<Ship>> &SelectedEscorts() const;
	// Select any of the ships that the player owns that are located within the given box
	// on screen. Return true if any were selected.
	bool SelectEscorts(const Rectangle &box, bool hasShift);
	// Select ships in the given stack, provided by the EscortDisplay. All ships in the stack should share the same
	// sprite. If the player clicks on the same escort icon multiple times, the ship targeted by the player's flagship
	// will cycle through the stack. Only escorts that the player owns will be selected for the giving of orders.
	void SelectShips(const std::vector<std::weak_ptr<Ship>> &stack, bool hasShift);
	// Select one of the ships that the player owns.
	void SelectEscort(const Ship *ship, bool hasShift);
	void DeselectEscort(const Ship *ship);
	void SelectEscortGroup(int group, bool hasShift);
	void SetEscortGroup(int group, const std::set<Ship *> *newShips = nullptr);
	std::set<Ship *> GetEscortGroup(int group);

	// Keep track of any outfits that you have sold since landing. These will be
	// available to buy back until you take off.
	const std::map<const Outfit*, int> &GetStock() const;
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
	// Should help dialogs relating to carriers be displayed?
	bool DisplayCarrierHelp() const;

	// Advance any active mission timers that meet the right criteria.
	void StepMissionTimers(UI *ui);
	// Checks and resets recacheJumpRoutes. Returns the value that was present upon entry.
	bool RecacheJumpRoutes();


private:
	class ScheduledEvent {
	public:
		// For loading a future event from the save file.
		ScheduledEvent(const DataNode &node, const ConditionsStore *playerConditions);
		// For scheduling a new event.
		ScheduledEvent(GameEvent event, Date date);

		// Comparison operator, based on the scheduled date of the event.
		bool operator<(const ScheduledEvent &other) const;

		ExclusiveItem<GameEvent> event;
		Date date;
	};


private:
	// Apply any "changes" saved in this player info to the global game state.
	void ApplyChanges();
	// After loading & applying changes, make sure the player & ship locations are sensible.
	void ValidateLoad();
	// Helper to register derived conditions.
	void RegisterDerivedConditions();

	// Helper for triggering events.
	void TriggerEvent(GameEvent event, std::list<DataNode> &eventChanges);

	// New missions are generated each time you land on a planet.
	void CreateMissions();
	void StepMissions(UI *ui);
	void Autosave() const;
	void Save(const std::string &path) const;
	void Save(DataWriter &out) const;

	// Check for and apply any punitive actions from planetary security.
	void Fine(UI *ui);

	// Set the flagship (on departure or during flight).
	void SetFlagship(Ship &other);

	void HandleFlagshipParking(Ship *oldFirstShip, Ship *newFirstShip);

	// Helper function to update the ship selection.
	void SelectEscort(const std::shared_ptr<Ship> &ship, bool *first);

	// Instantiate the given model and add it to the player's fleet.
	void AddStockShip(const Ship *model, const std::string &name);
	// When we remove a ship, forget it's stored Uuid.
	void ForgetGiftedShip(const Ship &oldShip, bool failsMissions = true);

	// Check that this player's current state can be saved.
	bool CanBeSaved() const;
	// Handle the daily salaries and payments.
	void DoAccounting();

	bool HasClearance() const;


private:
	std::string firstName;
	std::string lastName;
	std::string filePath;

	Date date;
	SystemEntry entry = SystemEntry::TAKE_OFF;
	const System *previousSystem = nullptr;
	const System *system = nullptr;
	const Planet *planet = nullptr;
	bool shouldLaunch = false;
	bool isDead = false;
	bool displayCarrierHelp = false;

	// The amount of in-game time played, in seconds.
	double playTime = 0.;

	Account accounts;
	// The licenses that the player owns.
	std::set<std::string> licenses;

	std::shared_ptr<Ship> flagship;
	std::vector<std::shared_ptr<Ship>> ships;
	std::vector<std::weak_ptr<Ship>> selectedEscorts;
	std::map<const Ship *, int> groups;
	CargoHold cargo;
	std::map<const Planet *, CargoHold> planetaryStorage;
	std::map<std::string, int64_t> costBasis;

	std::map<Date, BookEntry> logbook;
	std::map<std::string, std::map<std::string, BookEntry>> specialLogs;

	// A list of the player's active, accepted missions.
	std::list<Mission> missions;
	// These lists are populated when you land on a planet, and saved so that
	// they will not change if you reload the game.
	std::list<Mission> availableJobs;
	std::list<Mission> availableMissions;
	// This list is populated upon entering a system, and isn't saved since
	// you can't save in space.
	std::list<Mission> availableEnteringMissions;
	// This list is populated upon boarding a ship, and isn't saved since
	// you can't save in space. As of right now, only one boarding mission
	// can be offered at a time, so this list will only ever contain one or
	// zero missions.
	std::list<Mission> availableBoardingMissions;
	// If any mission component is not fully defined, the mission is deactivated
	// until its components are fully evaluable (i.e. needed plugins are reinstalled).
	std::list<Mission> inactiveMissions;
	// If any past event is not fully defined, the player should be warned that
	// the universe may not be in the expected state.
	std::set<std::string> invalidEvents;
	// Missions that are failed or aborted, but not yet deleted, and any
	// missions offered while in-flight are not saved.
	std::list<Mission> doneMissions;
	// This pointer to the most recently accepted boarding/assisting/entering mission
	// enables its NPCs to be placed before the player lands, and is then cleared.
	Mission *activeInFlightMission = nullptr;
	// For each active mission with a deadline, calculate how many days the player
	// has left to complete the mission. The number of days remaining is reduced
	// by the number of days of travel it will take to complete the mission if the
	// "Deadline blink by distance" preference is true.
	std::map<const Mission *, int> remainingDeadlines;
	// How to sort availableJobs
	bool availableSortAsc = true;
	SortType availableSortType;
	bool sortSeparateDeadline = false;
	bool sortSeparatePossible = false;

	ConditionsStore conditions;
	std::map<std::string, EsUuid> giftedShips;

	std::set<const System *> seen;
	std::set<const System *> visitedSystems;
	std::set<const Planet *> visitedPlanets;
	std::vector<const System *> travelPlan;
	const Planet *travelDestination = nullptr;

	std::set<const Outfit *> selectedWeapons;

	std::map<const Outfit *, int> stock;
	Depreciation depreciation;
	Depreciation stockDepreciation;
	std::set<std::pair<const System *, const Outfit *>> harvested;

	// Changes that this PlayerInfo wants to make to the global galaxy state:
	std::vector<std::pair<const Government *, double>> reputationChanges;
	std::map<const Planet *, int64_t> tributeReceived;
	std::list<DataNode> dataChanges;
	DataNode economy;
	// Persons that have been killed in this player's universe:
	std::vector<std::string> destroyedPersons;
	// Events that are going to happen some time in the future (sorted by date for easy chronological access):
	std::multiset<ScheduledEvent> scheduledEvents;
	// The names of events that were triggered in the past. Only needed when loading the game to determine
	// if an invalid event is referenced in the save file that the player should be warned about.
	std::set<std::string> triggeredEvents;
	// Whether a date note has already been added to dataChanges to mark when new changes have occurred.
	bool markedChangesToday = false;

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

	std::unique_ptr<DataWriter> transactionSnapshot;

	bool recacheJumpRoutes = false;
};
