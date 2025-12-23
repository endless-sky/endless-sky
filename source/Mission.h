/* Mission.h
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

#include "Color.h"
#include "ConditionSet.h"
#include "Date.h"
#include "DistanceCalculationSettings.h"
#include "EsUuid.h"
#include "ExclusiveItem.h"
#include "LocationFilter.h"
#include "MissionAction.h"
#include "MissionTimer.h"
#include "NPC.h"
#include "TextReplacements.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

class ConditionsStore;
class DataNode;
class DataWriter;
class Planet;
class PlayerInfo;
class Ship;
class ShipEvent;
class System;
class UI;



// This class represents a mission that the player can take on, such as carrying
// cargo from one planet to another or defending or attacking a ship. Many of
// a mission's parameters can be randomized, either so that the same mission
// template can be reused many times, or just so the mission is not always
// exactly the same every time you replay the game.
class Mission {
public:
	Mission() = default;
	// Copying a mission instance isn't allowed.
	Mission(const Mission &) = delete;
	Mission &operator=(const Mission &) = delete;
	Mission(Mission &&) = default;
	Mission &operator=(Mission &&) = default;
	~Mission() noexcept = default;

	// Construct and Load() at the same time.
	explicit Mission(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);

	// Load a mission, either from the game data or from a saved game.
	void Load(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);
	// Save a mission. It is safe to assume that any mission that is being saved
	// is already "instantiated," so only a subset of the data must be saved.
	void Save(DataWriter &out, const std::string &tag = "mission") const;
	// Add "never" to the toOffer ConditionSet, preventing this mission from offering.
	void NeverOffer();

	// Basic mission information.
	const EsUuid &UUID() const noexcept;
	// Get the internal name used for this mission. This name is unique and is
	// never modified by string substitution, so it can be used in condition
	// variables, etc.
	const std::string &TrueName() const;
	const std::string &DisplayName() const;
	const std::string &Description() const;
	// Check if this mission should be shown in your mission list. If not, the
	// player will not know this mission exists (which is sometimes useful).
	bool IsVisible() const;
	// The colors that should be used to display the mission name if it is shown
	// in your mission list.
	const Color &Unavailable() const;
	const Color &Unselected() const;
	const Color &Selected() const;
	// Check if this mission should be quarantined due to requiring currently-
	// undefined ships, planets, or systems (i.e. is from an inactive plugin).
	bool IsValid() const;
	// Check if this mission has high priority. If any priority missions
	// are available, only other priority missions and non-blocking ones can offer alongside it.
	bool HasPriority() const;
	// Check if this mission is a "non-blocking" mission.
	// Such missions will not prevent minor missions from being offered alongside them,
	// and will not be prevented from offering by priority missions.
	bool IsNonBlocking() const;
	// Check if this mission is a "minor" mission. Minor missions will only be
	// offered if no other non-blocking missions (minor or otherwise) are being offered.
	bool IsMinor() const;
	int OfferPrecedence() const;

	// Find out where this mission is offered.
	enum Location {SPACEPORT, LANDING, JOB, ASSISTING, BOARDING, SHIPYARD, OUTFITTER, JOB_BOARD, ENTERING};
	bool IsAtLocation(Location location) const;

	// Information about what you are doing.
	const Ship *SourceShip() const;
	const Planet *Destination() const;
	const std::set<const System *> &Waypoints() const;
	const std::set<const System *> &VisitedWaypoints() const;
	const std::set<const Planet *> &Stopovers() const;
	const std::set<const Planet *> &VisitedStopovers() const;
	const std::set<const System *> &MarkedSystems() const;
	const std::set<const System *> &UnmarkedSystems() const;
	const std::set<const System *> &TrackedSystems() const;
	void RecalculateTrackedSystems();
	void Mark(const std::set<const System *> &systems) const;
	void Unmark(const std::set<const System *> &system) const;
	const std::string &Cargo() const;
	int CargoSize() const;
	int Fine() const;
	std::string FineMessage() const;
	bool FailIfDiscovered() const;
	int Passengers() const;
	int64_t DisplayedPayment() const;
	// The mission should take this many jumps.
	// Only matters to available jobs (not saved to file)
	const int ExpectedJumps() const;
	int CalculateJumps(const System *const sourceSystem);
	// The mission must be completed by this deadline (if there is a deadline).
	const Date &Deadline() const;
	// If this mission's deadline was before the given date and it has not been
	// marked as failing already, mark it and return true.
	bool CheckDeadline(const Date &today);
	// Check if you have special clearance to land on your destination.
	bool HasClearance(const Planet *planet) const;
	// Get the string to be shown in the destination planet's hailing dialog. If
	// this is "auto", you don't have to hail them to get landing permission.
	const std::string &ClearanceMessage() const;
	// Check whether we have full clearance to land and use the planet's
	// services, or whether we are landing in secret ("infiltrating").
	bool HasFullClearance() const;

	// Check if it's possible to offer or complete this mission right now. The
	// check for whether you can offer a mission does not take available space
	// into account, so before actually offering a mission you should also check
	// if the player has enough space.
	bool CanOffer(const PlayerInfo &player, const std::shared_ptr<Ship> &boardingShip = nullptr) const;
	bool CanAccept(const PlayerInfo &player) const;
	bool HasSpace(const PlayerInfo &player) const;
	bool HasSpace(const Ship &ship) const;
	bool CanComplete(const PlayerInfo &player) const;
	bool IsSatisfied(const PlayerInfo &player) const;
	bool IsFailed() const;
	bool OverridesCapture() const;
	// Mark a mission failed (e.g. due to a "fail" action in another mission).
	void Fail();
	// Get a string to show if this mission is "blocked" from being offered
	// because it requires you to have more passenger or cargo space free. After
	// calling this function, any future calls to it will return an empty string
	// so that you do not display the same message multiple times.
	std::string BlockedMessage(const PlayerInfo &player);
	// Check if this mission recommends that the game be autosaved when it is
	// accepted. This should be set for main story line missions that have a
	// high chance of failing, such as escort missions.
	bool RecommendsAutosave() const;
	// Check if this mission is unique, i.e. not something that will be offered
	// over and over again in different variants.
	bool IsUnique() const;

	// When the state of this mission changes, it may make changes to the player
	// information or show new UI panels. PlayerInfo::MissionCallback() will be
	// used as the callback for an `on offer` conversation, to handle its response.
	// If it is not possible for this change to happen, this function returns false.
	enum Trigger {COMPLETE, OFFER, ACCEPT, DECLINE, FAIL, ABORT, DEFER, VISIT, STOPOVER, WAYPOINT, DAILY, DISABLED};
	bool Do(Trigger trigger, PlayerInfo &player, UI *ui = nullptr, const std::shared_ptr<Ship> &boardingShip = nullptr);

	// Get a list of NPCs associated with this mission. Every time the player
	// takes off from a planet, they should be added to the active ships.
	const std::list<NPC> &NPCs() const;
	// Iterate through the timers and progress them if applicable.
	void StepTimers(PlayerInfo &player, UI *ui);
	// Update which NPCs are active based on their spawn and despawn conditions.
	void UpdateNPCs(const PlayerInfo &player);
	// Checks if the given ship belongs to one of the mission's NPCs.
	bool HasShip(const std::shared_ptr<Ship> &ship) const;
	// If any event occurs between two ships, check to see if this mission cares
	// about it. This may affect the mission status or display a message.
	void Do(const ShipEvent &event, PlayerInfo &player, UI *ui);
	bool RequiresGiftedShip(const std::string &shipId) const;

	// Get a specific mission action from this mission.
	// If the mission action is not found for the given trigger, returns an empty
	// mission action.
	const MissionAction &GetAction(Trigger trigger) const;

	// "Instantiate" a mission by replacing randomly selected values and places
	// with a single choice, and then replacing any wildcard text as well.
	Mission Instantiate(const PlayerInfo &player, const std::shared_ptr<Ship> &boardingShip = nullptr) const;


private:
	bool Enter(const System *system, PlayerInfo &player, UI *ui);
	// For legacy code, contraband definitions can be placed in two different
	// locations, so move that parsing out to a helper function.
	bool ParseContraband(const DataNode &node);


private:
	std::string trueName;
	std::string displayName;
	std::string description;
	std::string blocked;
	Location location = SPACEPORT;

	// Colors that determine how this mission displays in the MissionPanel.
	ExclusiveItem<Color> unavailable;
	ExclusiveItem<Color> unselected;
	ExclusiveItem<Color> selected;

	EsUuid uuid;

	bool hasFailed = false;
	bool isVisible = true;
	bool hasPriority = false;
	bool isNonBlocking = false;
	bool isMinor = false;
	// By default, missions are offered in alphabetical order.
	// Using a non-zero offer precedence changes the order that missions are offered in.
	// Missions with a higher offer precedence are offered before missions with a lower
	// precedence. Where two missions have the same offer precedence, they continue to
	// offer in alphabetical order.
	int offerPrecedence = 0;
	bool autosave = false;
	bool overridesCapture = false;
	Date deadline;
	int expectedJumps = 0;
	int deadlineBase = 0;
	int deadlineMultiplier = 0;
	DistanceCalculationSettings distanceCalcSettings;
	std::string clearance;
	bool ignoreClearance = false;
	LocationFilter clearanceFilter;
	bool hasFullClearance = true;
	LocationFilter completionFilter;

	int repeat = 1;
	std::string cargo;
	int cargoSize = 0;
	// Parameters for generating random cargo amounts:
	int cargoLimit = 0;
	double cargoProb = 0.;
	int fine = 0;
	std::string fineMessage;
	bool failIfDiscovered = false;
	int passengers = 0;
	// Parameters for generating random passenger amounts:
	int passengerLimit = 0;
	double passengerProb = 0.;
	int64_t paymentApparent = 0;

	ConditionSet toOffer;
	ConditionSet toAccept;
	ConditionSet toComplete;
	ConditionSet toFail;

	const Planet *source = nullptr;
	// The ship this mission originated from, if it is a boarding mission.
	const Ship *sourceShip = nullptr;
	LocationFilter sourceFilter;
	const Planet *destination = nullptr;
	LocationFilter destinationFilter;
	// Systems that must be visited:
	std::set<const System *> waypoints;
	std::list<LocationFilter> waypointFilters;
	std::set<const Planet *> stopovers;
	std::list<LocationFilter> stopoverFilters;
	std::set<const Planet *> visitedStopovers;
	std::set<const System *> visitedWaypoints;
	// Systems that don't need to be visited, but which the mission still
	// wants to highlight for the player.
	mutable std::set<const System *> markedSystems;
	mutable std::set<const System *> unmarkedSystems;
	// Systems that are marked because a "tracked" mission NPC is in them.
	bool hasTrackedNpcs = false;
	std::set<const System *> trackedSystems;

	// User-defined text replacements unique to this mission:
	TextReplacements substitutions;

	// NPCs:
	std::list<NPC> npcs;
	// Timers:
	std::list<MissionTimer> timers;

	// Actions to perform:
	std::map<Trigger, MissionAction> actions;
	// "on enter" actions may name a specific system, or rely on matching a
	// LocationFilter in order to designate the matched system.
	std::map<const System *, MissionAction> onEnter;
	std::list<MissionAction> genericOnEnter;
	// Track which `on enter` MissionActions have triggered.
	std::set<const MissionAction *> didEnter;
};
