/* MissionAction.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MISSION_ACTION_H_
#define MISSION_ACTION_H_

#include "ConditionSet.h"
#include "Conversation.h"
#include "LocationFilter.h"
#include "Phrase.h"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class DataWriter;
class GameEvent;
class Outfit;
class PlayerInfo;
class Ship;
class System;
class UI;



// A MissionAction represents what happens when a mission reaches a certain
// milestone: offered, accepted, declined, completed or failed. Actions might
// include showing a dialog or conversation, giving the player payment or a
// special item, modifying condition flags, or queueing an event to occur.
class MissionAction {
public:
	MissionAction() = default;
	// Construct and Load() at the same time.
	MissionAction(const DataNode &node, const std::string &missionName);
	
	void Load(const DataNode &node, const std::string &missionName);
	// Note: the Save() function can assume this is an instantiated mission, not
	// a template, so it only has to save a subset of the data.
	void Save(DataWriter &out) const;
	// An invalid MissionAction references content that is not fully defined.
	bool IsValid() const;
	
	int Payment() const;
	
	const std::string &DialogText() const;
	
	// Check if this action can be completed right now. It cannot be completed
	// if it takes away money or outfits that the player does not have, or should
	// take place in a system that does not match the specified LocationFilter.
	bool CanBeDone(const PlayerInfo &player, const std::shared_ptr<Ship> &boardingShip = nullptr) const;
	// Perform this action. If a conversation is shown, the given destination
	// will be highlighted in the map if you bring it up.
	void Do(PlayerInfo &player, UI *ui = nullptr, const System *destination = nullptr, const std::shared_ptr<Ship> &ship = nullptr, const bool isUnique = true) const;
	
	// "Instantiate" this action by filling in the wildcard text for the actual
	// destination, payment, cargo, etc.
	MissionAction Instantiate(std::map<std::string, std::string> &subs, const System *origin, int jumps, int64_t payload) const;
	
	
private:
	std::string trigger;
	std::string system;
	LocationFilter systemFilter;
	
	std::string logText;
	std::map<std::string, std::map<std::string, std::string>> specialLogText;
	
	std::string dialogText;
	const Phrase *stockDialogPhrase = nullptr;
	Phrase dialogPhrase;
	
	const Conversation *stockConversation = nullptr;
	Conversation conversation;
	
	std::map<const GameEvent *, std::pair<int, int>> events;
	std::vector<std::pair<const Ship *, std::string>> giftShips;
	std::map<const Outfit *, int> giftOutfits;
	std::map<const Outfit *, int> requiredOutfits;
	int64_t payment = 0;
	int64_t paymentMultiplier = 0;
	
	// When this action is performed, the missions with these names fail.
	std::set<std::string> fail;
	
	ConditionSet conditions;
};



#endif
