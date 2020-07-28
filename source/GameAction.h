/* GameAction.h
Copyright (c) 2020 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_ACTION_H_
#define GAME_ACTION_H_

#include "ConditionSet.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

class DataNode;
class DataWriter;
class GameEvent;
class PlayerInfo;



// A MissionAction represents what happens when a mission reaches a certain
// milestone: offered, accepted, declined, completed or failed. Actions might
// include showing a dialog or conversation, giving the player payment or a
// special item, modifying condition flags, or queueing an event to occur.
class GameAction {
public:
	GameAction() = default;
	// Construct and Load() at the same time.
	GameAction(const DataNode &node, const std::string &missionName);
	
	void Load(const DataNode &node, const std::string &missionName);
	void LoadSingle(const DataNode &child, const std::string &missionName);
	void Save(DataWriter &out) const;
	
	bool IsEmpty() const;
	
	int Payment() const;
	
	// Perform this action.
	void Do(PlayerInfo &player, bool conversation = false) const;
	
	// "Instantiate" this action by filling in the wildcard text for the actual
	// destination, payment, cargo, etc.
	GameAction Instantiate(std::map<std::string, std::string> &subs, int jumps, int payload) const;
	
	
private:
	bool empty = true;
	
	std::string logText;
	std::map<std::string, std::map<std::string, std::string>> specialLogText;
	
	std::map<const GameEvent *, std::pair<int, int>> events;
	int64_t payment = 0;
	int64_t paymentMultiplier = 0;
	
	// When this action is performed, the missions with these names fail.
	std::set<std::string> fail;
	
	ConditionSet conditions;
};



#endif
