/* Milestone.cpp
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

#include "Milestone.h"

#include "ConditionsStore.h"
#include "DataNode.h"
#include "GameData.h"
#include "PlayerInfo.h"

using namespace std;

namespace {
	static map<string, MilestoneState> stringToMilestoneState = {
		{"default", MilestoneState::DEFAULT},
		{"hidden", MilestoneState::HIDDEN},
		{"locked", MilestoneState::LOCKED},
		{"unlocked", MilestoneState::UNLOCKED},
		{"completed", MilestoneState::COMPLETE},
		{"blocked", MilestoneState::BLOCKED}
	};

}



Milestone::Milestone()
{
}



void Milestone::Load(const DataNode &node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("Error: No name specified for milestone.");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "hidden")
			isHidden = true;
		else if(child.Token(0) == "locked")
			isLocked = true;
		else if(child.Size() >= 2)
		{
			if(child.Token(0) == "to")
			{
				if(child.Token(1) == "unhide")
					toUnhide.Load(child);
				else if(child.Token(1) == "unlock")
					toUnlock.Load(child);
				else if(child.Token(1) == "complete")
					toComplete.Load(child);
				else if(child.Token(1) == "block")
					toBlock.Load(child);
			}
			else if(child.Size() >= 3)
			{
				if(child.Token(0) == "locked")
					locked = make_pair(child.Token(1), child.Token(2));
				else if(child.Token(0) == "unlocked")
					unlocked = make_pair(child.Token(1), child.Token(2));
				else if(child.Token(0) == "completed")
					completed = make_pair(child.Token(1), child.Token(2));
			}
		}
	}

	if(isHidden || !toUnhide.IsEmpty())
		initialState = MilestoneState::HIDDEN;
	else if(isLocked || !toUnlock.IsEmpty())
		initialState = MilestoneState::LOCKED;
	else
		initialState = MilestoneState::UNLOCKED;
}



void Milestone::UpdateMilestones(map<string, MilestoneState> &playerMilestones, const ConditionsStore &playerConditions)
{
	for(const auto &it : GameData::Milestones())
	{
		const string &name = it.first;
		const auto &milestoneIt = playerMilestones.find(name);
		if(milestoneIt != playerMilestones.end())
			milestoneIt->second = it.second.CheckState(playerConditions, milestoneIt->second);
		else
		{
			MilestoneState result = it.second.CheckState(playerConditions, MilestoneState::DEFAULT);
			if(result != it.second.initialState)
				playerMilestones[it.first] = result;
		}
	}
}



const MilestoneState &Milestone::MilestoneStateFromString(const string &name)
{
	const auto &it = stringToMilestoneState.find(name);
	if(it == stringToMilestoneState.end())
		return stringToMilestoneState["default"];
	return it->second;
}



const MilestoneState Milestone::CheckState(const ConditionsStore &conditions, MilestoneState currentState) const
{
	if(currentState == MilestoneState::BLOCKED || toBlock.Test(conditions))
		return MilestoneStateFromString("blocked");
	if(currentState == MilestoneState::DEFAULT)
		currentState = initialState;
	if(currentState == MilestoneState::HIDDEN && toUnhide.Test(conditions))
		currentState = MilestoneState::LOCKED;
	if(currentState == MilestoneState::LOCKED && toUnlock.Test(conditions))
		currentState = MilestoneState::UNLOCKED;
	if(currentState == MilestoneState::UNLOCKED && toComplete.Test(conditions))
		currentState = MilestoneState::COMPLETE;
	return currentState;
}





