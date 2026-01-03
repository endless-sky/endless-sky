/* NPC.h
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

#include "Conversation.h"
#include "EsUuid.h"
#include "ExclusiveItem.h"
#include "Fleet.h"
#include "FleetCargo.h"
#include "LocationFilter.h"
#include "NPCAction.h"
#include "Personality.h"
#include "Phrase.h"

#include <list>
#include <map>
#include <memory>
#include <string>

class ConditionsStore;
class DataNode;
class DataWriter;
class Government;
class Mission;
class Planet;
class PlayerInfo;
class Ship;
class ShipEvent;
class System;
class UI;



// An NPC is a ship associated with a mission. Certain required actions are
// associated with each NPC, such as boarding it, killing it, or making sure it
// is not boarded or killed. NPCs also have different "behaviors," such as
// staying in the system they started in, or attacking only the player's ships.
class NPC {
public:
	enum class Trigger : int_fast8_t {
		// Triggers corresponding directly to ShipEvents.
		ASSIST,
		SCAN_CARGO,
		SCAN_OUTFITS,
		PROVOKE,
		DISABLE,
		BOARD,
		CAPTURE,
		DESTROY,
		ENCOUNTER,
		// Can be triggered by either the CAPTURE or DESTROY events.
		KILL,
	};


public:
	NPC() = default;
	// Copying an NPC instance isn't allowed.
	NPC(const NPC &) = delete;
	NPC &operator=(const NPC &) = delete;
	NPC(NPC &&) noexcept = default;
	NPC &operator=(NPC &&) noexcept = default;
	~NPC() noexcept = default;

	// Construct and Load() at the same time.
	explicit NPC(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);

	void Load(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);
	// Note: the Save() function can assume this is an instantiated mission, not
	// a template, so fleets will be replaced by individual ships already.
	void Save(DataWriter &out) const;

	// Determine if this NPC or NPC template uses well-defined data.
	// Returns the reason the NPC is not valid, or an empty string if valid.
	std::string Validate(bool asTemplate = false) const;

	const EsUuid &UUID() const noexcept;

	// Update or check spawning and despawning for this NPC.
	void UpdateSpawning(const PlayerInfo &player);
	bool ShouldSpawn() const;

	// Get the personality that dictates the behavior of the ships associated with this set of NPCs.
	const Personality &GetPersonality() const;
	// Get the ships associated with this set of NPCs.
	const std::list<std::shared_ptr<Ship>> Ships() const;

	// Handle the given ShipEvent. Return true if the event target is within this NPC.
	bool Do(const ShipEvent &event, PlayerInfo &player, UI *ui = nullptr,
		const Mission *caller = nullptr, bool isVisible = true);
	// Determine if the NPC is in a successful state, assuming the player is in the given system.
	// (By default, a despawnable NPC has succeeded and is not actually checked.)
	bool HasSucceeded(const System *playerSystem, bool ignoreIfDespawnable = true) const;
	// Check if the NPC is supposed to be accompanied and is not.
	bool IsLeftBehind(const System *playerSystem) const;
	// Determine if the NPC is in a failed state. A failed state is irrecoverable, except for
	// NPCs which would despawn upon the player's next landing.
	bool HasFailed() const;

	// Create a copy of this NPC but with the fleets replaced by the actual
	// ships they represent, wildcards in the conversation text replaced, etc.
	NPC Instantiate(const PlayerInfo &player, std::map<std::string, std::string> &subs, const System *origin,
			const System *destination, int jumps, int64_t payload) const;


private:
	// Handle any NPC mission actions that may have been triggered by a ShipEvent.
	void DoActions(const ShipEvent &event, bool newEvent, PlayerInfo &player, UI *ui, const Mission *caller);


private:
	// The government of the ships in this NPC:
	const Government *government = nullptr;
	Personality personality;

	// The cargo ships in this NPC will be able to carry.
	FleetCargo cargo;
	bool overrideFleetCargo = false;

	EsUuid uuid;

	// Start out in a location matching this filter, or in a particular system:
	LocationFilter location;
	const System *system = nullptr;
	bool isAtDestination = false;
	// Start out landed on this planet.
	const Planet *planet = nullptr;

	// Dialog or conversation to show when all requirements for this NPC are met:
	std::string dialogText;
	ExclusiveItem<Phrase> dialogPhrase;
	ExclusiveItem<Conversation> conversation;

	// Conditions that must be met in order for this NPC to be placed or despawned:
	ConditionSet toSpawn;
	ConditionSet toDespawn;
	// Once true, the NPC will be spawned on takeoff and its success state will influence
	// the parent mission's ability to be completed.
	bool passedSpawnConditions = false;
	// Once true, the NPC will be despawned on landing and it will no longer contribute to
	// the parent mission's ability to be completed or failed.
	bool passedDespawnConditions = false;
	// Whether we have actually checked spawning conditions yet. (This
	// will generally be true, except when reloading a save.)
	bool checkedSpawnConditions = false;

	// The ships may be listed individually or referred to as a fleet, and may
	// be customized or just refer to stock objects:
	std::list<std::shared_ptr<Ship>> ships;
	std::list<const Ship *> stockShips;
	std::list<std::string> shipNames;
	std::list<ExclusiveItem<Fleet>> fleets;

	// This must be done to each ship in this set to complete the mission:
	int succeedIf = 0;
	int failIf = 0;
	bool mustEvade = false;
	bool mustAccompany = false;
	// The ShipEvent actions that have been done to each ship.
	std::map<const Ship *, int> shipEvents;

	// The NPCActions that this NPC can run on certain events/triggers.
	std::map<Trigger, NPCAction> npcActions;
};
