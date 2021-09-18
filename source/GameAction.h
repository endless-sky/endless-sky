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

#include <cstdint>
#include <map>
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
class UI;



// A GameAction represents what happens when a Mission or Conversation reaches
// a certain milestone. This can include when the Mission is offered, accepted,
// declined, completed, or failed, or when a Conversation reaches an "action" node.
// GameActions might include giving the player payment or a special item,
// modifying condition flags, or queueing a GameEvent to occur. Any new mechanics
// added to GameAction should be able to be safely executed while in a
// Conversation.
class GameAction {
public:
	GameAction() = default;
	// Construct and Load() at the same time.
	GameAction(const DataNode &node, const std::string &missionName);
	
	void Load(const DataNode &node, const std::string &missionName);
	// Load a single child at a time, used for streamlining MissionAction::Load.
	void LoadAction(const DataNode &child, const std::string &missionName, bool conversation = true);
	void SaveAction(DataWriter &out) const;
	// Determine if this GameAction references content that is not fully defined.
	std::string ValidateAction() const;
	
	// If this action has not been loaded, then it is empty.
	bool IsEmpty() const;
	
	// Perform this action.
	void DoAction(PlayerInfo &player, UI *ui = nullptr) const;
	
	// "Instantiate" this action by filling in the wildcard data for the actual
	// payment, event delay, etc.
	GameAction Instantiate(std::map<std::string, std::string> &subs, int jumps, int payload) const;
	
	
protected:
	// Instantiate the data that is specific to a GameAction but not a MissionAction.
	void InstantiateAction(GameAction &result, std::map<std::string, std::string> &subs, int jumps, int payload) const;
	
	
protected:
	std::string logText;
	std::map<std::string, std::map<std::string, std::string>> specialLogText;
	
	std::map<const GameEvent *, std::pair<int, int>> events;
	std::vector<std::pair<const Ship *, std::string>> giftShips;
	std::map<const Outfit *, int> giftOutfits;
	// A GameAction contains a map of required outfits, but only a MissionAction
	// will populate it. This is for the purpose of catching old mission syntax
	// "outfit <outfit> 0" now being "require <outfit>".
	std::map<const Outfit *, int> requiredOutfits;
	int64_t payment = 0;
	int64_t paymentMultiplier = 0;
	int64_t fine = 0;
	
	// When this action is performed, the missions with these names fail.
	std::set<std::string> fail;
	
	ConditionSet conditions;
	
	
private:
	bool empty = true;
};



#endif
