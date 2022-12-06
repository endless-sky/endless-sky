/* MissionAction.h
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

#ifndef MISSION_ACTION_H_
#define MISSION_ACTION_H_

#include "Conversation.h"
#include "ExclusiveItem.h"
#include "GameAction.h"
#include "LocationFilter.h"
#include "Phrase.h"

#include <map>
#include <memory>
#include <string>

class DataNode;
class DataWriter;
class Outfit;
class PlayerInfo;
class System;
class UI;



// A MissionAction represents what happens when a Mission reaches a certain
// milestone, including offered, accepted, declined, completed, or failed.
// In addition to performing a GameAction, a MissionAction can gate the task on
// the ownership of specific outfits and also display dialogs or conversations.
class MissionAction {
public:
	MissionAction() = default;
	// Construct and Load() at the same time.
	MissionAction(const DataNode &node, const std::string &missionName);

	void Load(const DataNode &node, const std::string &missionName);
	// Note: the Save() function can assume this is an instantiated mission, not
	// a template, so it only has to save a subset of the data.
	void Save(DataWriter &out) const;
	// Determine if this MissionAction references content that is not fully defined.
	std::string Validate() const;

	const std::string &DialogText() const;

	// Check if this action can be completed right now. It cannot be completed
	// if it takes away money or outfits that the player does not have, or should
	// take place in a system that does not match the specified LocationFilter.
	bool CanBeDone(const PlayerInfo &player, const std::shared_ptr<Ship> &boardingShip = nullptr) const;
	// Perform this action. If a conversation is shown, the given destination
	// will be highlighted in the map if you bring it up.
	void Do(PlayerInfo &player, UI *ui = nullptr, const System *destination = nullptr,
		const std::shared_ptr<Ship> &ship = nullptr, const bool isUnique = true) const;

	// "Instantiate" this action by filling in the wildcard text for the actual
	// destination, payment, cargo, etc.
	MissionAction Instantiate(std::map<std::string, std::string> &subs,
		const System *origin, int jumps, int64_t payload) const;

	int64_t Payment() const noexcept;

private:
	std::string trigger;
	std::string system;
	LocationFilter systemFilter;

	std::string dialogText;
	ExclusiveItem<Phrase> dialogPhrase;
	ExclusiveItem<Conversation> conversation;

	// Outfits that are required to be owned (or not) for this action to be performable.
	std::map<const Outfit *, int> requiredOutfits;

	// Tasks this mission action performs, such as modifying accounts, inventory, or conditions.
	GameAction action;
};



#endif
