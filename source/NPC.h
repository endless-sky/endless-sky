/* NPC.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef NPC_H_
#define NPC_H_

#include "Conversation.h"
#include "Fleet.h"
#include "LocationFilter.h"
#include "Personality.h"
#include "Phrase.h"

#include <list>
#include <map>
#include <memory>
#include <string>

class DataNode;
class DataWriter;
class Government;
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
	NPC() = default;
	// Construct and Load() at the same time.
	NPC(const DataNode &node);
	
	void Load(const DataNode &node);
	// Note: the Save() function can assume this is an instantiated mission, not
	// a template, so fleets will be replaced by individual ships already.
	void Save(DataWriter &out) const;
	
	// Update or check spawning and despawning for this NPC.
	void UpdateSpawning(const PlayerInfo &player);
	bool PassedSpawn() const;
	bool PassedDespawn() const;
	
	// Get the ships associated with this set of NPCs.
	const std::list<std::shared_ptr<Ship>> Ships() const;
	
	// Handle the given ShipEvent.
	void Do(const ShipEvent &event, PlayerInfo &player, UI *ui = nullptr, bool isVisible = true);
	bool HasSucceeded(const System *playerSystem) const;
	// Check if the NPC is supposed to be accompanied and is not.
	bool IsLeftBehind(const System *playerSystem) const;
	bool HasFailed() const;
	
	// Create a copy of this NPC but with the fleets replaced by the actual
	// ships they represent, wildcards in the conversation text replaced, etc.
	NPC Instantiate(std::map<std::string, std::string> &subs, const System *origin, const System *destination) const;
	
	
private:
	// The government of the ships in this NPC:
	const Government *government = nullptr;
	Personality personality;
	
	// Start out in a location matching this filter, or in a particular system:
	LocationFilter location;
	const System *system = nullptr;
	bool isAtDestination = false;
	// Start out landed on this planet.
	const Planet *planet = nullptr;
	
	// Dialog or conversation to show when all requirements for this NPC are met:
	std::string dialogText;
	const Phrase *stockDialogPhrase = nullptr;
	Phrase dialogPhrase;
	
	Conversation conversation;
	const Conversation *stockConversation = nullptr;
	
	// Conditions that must be met in order for this NPC to be placed or despawned:
	bool passedSpawnConditions = false;
	bool passedDespawnConditions = false;
	ConditionSet toSpawn;
	ConditionSet toDespawn;
	
	// The ships may be listed individually or referred to as a fleet, and may
	// be customized or just refer to stock objects:
	std::list<std::shared_ptr<Ship>> ships;
	std::list<const Ship *> stockShips;
	std::list<std::string> shipNames;
	std::list<Fleet> fleets;
	std::list<const Fleet *> stockFleets;
	
	// This must be done to each ship in this set to complete the mission:
	int succeedIf = 0;
	int failIf = 0;
	bool mustEvade = false;
	bool mustAccompany = false;
	std::map<const Ship *, int> actions;
};



#endif
