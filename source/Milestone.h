/* Milestone.h
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MILESTONE_H_
#define MILESTONE_H_

#include "ConditionSet.h"
#include "milestonestate.hpp"

#include <map>
#include <utility>

class ConditionsStore;
class DataNode;
class PlayerInfo;



class Milestone {
public:
	Milestone();

	void Load(const DataNode &node);

	static void UpdateMilestones(std::map<std::string, MilestoneState> &playerMilestones, const ConditionsStore &playerConditions);

	static const MilestoneState &MilestoneStateFromString(const std::string &name);


private:
	const MilestoneState CheckState(const ConditionsStore &conditions, MilestoneState currentState) const;


private:
	std::string name;
	MilestoneState initialState = MilestoneState::DEFAULT;
	// Quiet milestones don't have a pop-up message when completed.
	bool isQuiet = false;

	// Hidden milestones are not shown in the list at all until
	// either the 'toUnhide' or 'toComplete' conditions are met.
	bool isHidden = false;
	ConditionSet toUnhide;
	// Locked milestones have an entry in the list but the name
	// and description may change once the 'toUnlock' conditions
	// are met.
	bool isLocked = false;
	std::pair<std::string, std::string> locked;
	ConditionSet toUnlock;
	std::pair<std::string, std::string> unlocked;
	ConditionSet toComplete;
	std::pair<std::string, std::string> completed;

	// If the 'toBlock' conditions are met, this milestone will
	// become permanently hidden.
	ConditionSet toBlock;


};

#endif
