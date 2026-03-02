/* GameAction.h
Copyright (c) 2020 by Amazinite

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

#include "BookEntry.h"
#include "ConditionAssignments.h"
#include "Message.h"
#include "ShipManager.h"

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class DataWriter;
class GameEvent;
class Mission;
class Outfit;
class PlayerInfo;
class Ship;
class System;
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
	GameAction(const DataNode &node, const ConditionsStore *playerConditions);

	void Load(const DataNode &node, const ConditionsStore *playerConditions);
	// Process a single sibling node.
	void LoadSingle(const DataNode &child, const ConditionsStore *playerConditions);
	void Save(DataWriter &out) const;

	// Determine if this GameAction references content that is not fully defined.
	std::string Validate() const;

	// Whether this action instance contains any tasks to perform.
	bool IsEmpty() const noexcept;

	int64_t Payment() const noexcept;
	int64_t Fine() const noexcept;
	const std::map<const Outfit *, int> &Outfits() const noexcept;
	const std::vector<ShipManager> &Ships() const noexcept;

	// Perform this action.
	void Do(PlayerInfo &player, UI *ui, const Mission *caller) const;

	// "Instantiate" this action by filling in the wildcard data for the actual
	// payment, event delay, etc.
	GameAction Instantiate(std::map<std::string, std::string> &subs, int jumps, int payload) const;


private:
	struct Debt {
		Debt(int64_t amount) : amount(amount) {}

		int64_t amount = 0;
		std::optional<double> interest;
		int term = 365;
	};


private:
	bool isEmpty = true;
	BookEntry logEntries;
	std::map<std::string, std::map<std::string, BookEntry>> specialLogEntries;
	std::map<std::string, std::vector<std::string>> specialLogClear;

	std::map<const GameEvent *, std::pair<int, int>> events;
	std::vector<ShipManager> giftShips;
	std::map<const Outfit *, int> giftOutfits;

	int64_t payment = 0;
	int64_t paymentMultiplier = 0;
	int64_t fine = 0;
	std::vector<Debt> debt;

	std::optional<std::string> music;

	std::set<const System *> mark;
	std::map<std::string, std::set<const System *>> markOther;
	std::set<const System *> unmark;
	std::map<std::string, std::set<const System *>> unmarkOther;

	// When this action is performed, the missions with these names fail.
	std::set<std::string> fail;
	// When this action is performed, the mission that called this action is failed.
	bool failCaller = false;

	std::vector<ExclusiveItem<Message>> messages;

	ConditionAssignments conditions;
};
