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
#include "DataNode.h"
#include "milestonestate.hpp"

#include <map>
#include <utility>

class Milestone {
public:
	Milestone(const DataNode &node);

	void Load(const DataNode &node);

	bool IsHidden() const;
	bool IsLocked() const;

	MilestoneState CheckState(std::map<std::string, int64_t> conditions, MilestoneState currentState);


private:
	std::string name;
	// Quiet milestones don't have a pop-up message when completed.
	bool isQuiet;

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

	std::map<MilestoneState, std::pair<std::string, std::string>> displayNamesAndDescs;
	// If the 'toBlock' conditions are met, this milestone will
	// become permanently hidden.
	ConditionSet toBlock;


};

#endif
