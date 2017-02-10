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

#include <map>
#include <set>
#include <string>

class DataNode;
class DataWriter;
class GameEvent;
class Outfit;
class PlayerInfo;
class System;
class UI;



// A MissionAction represents what happens when a mission reaches a certain
// milestone: offered, accepted, declined, completed or failed. Actions might
// include showing a dialog or conversation, giving the player payment or a
// special item, modifying condition flags, or queueing an event to occur.
class MissionAction {
public:
	void Load(const DataNode &node, const std::string &missionName);
	// Note: the Save() function can assume this is an instantiated mission, not
	// a template, so it only has to save a subset of the data.
	void Save(DataWriter &out) const;
	
	int Payment() const;
	// Tell this object what the default payment for this mission turned out to
	// be. It will ignore this information if it is not giving default payment.
	void SetDefaultPayment(int credits);
	
	// Check if this action can be completed right now. It cannot be completed
	// if it takes away money or outfits that the player does not have.
	bool CanBeDone(const PlayerInfo &player) const;
	// Perform this action. If a conversation is shown, the given destination
	// will be highlighted in the map if you bring it up.
	void Do(PlayerInfo &player, UI *ui = nullptr, const System *destination = nullptr) const;
	
	// "Instantiate" this action by filling in the wildcard text for the actual
	// destination, payment, cargo, etc.
	MissionAction Instantiate(std::map<std::string, std::string> &subs, int jumps, int payload) const;
	
	
private:
	std::string trigger;
	std::string system;
	
	std::string dialogText;
	
	const Conversation *stockConversation = nullptr;
	Conversation conversation;
	
	std::map<std::string, int> events;
	std::map<const Outfit *, int> gifts;
	int64_t payment = 0;
	int64_t paymentMultiplier = 0;
	
	// When this action is performed, the missions with these names fail.
	std::set<std::string> fail;
	
	ConditionSet conditions;
};



#endif
