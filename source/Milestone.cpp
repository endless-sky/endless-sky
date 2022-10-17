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

#include "DataNode.h"

using namespace std;

namespace {
	static map<string, MilestoneState> stringToMilestoneState = {
		{"hidden", MilestoneState::HIDDEN},
		{"locked", MilestoneState::LOCKED},
		{"unlocked", MilestoneState::UNLOCKED},
		{"completed", MilestoneState::COMPLETE}
	};

}



Milestone::Milestone(const DataNode &node)
{
	Load(node);
}



void Milestone::Load(const DataNode &node)
{
	if((node.Size() < 2))
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
			else if(child.Token(0) == "on")
			{

			}
			else
			{
				displayNamesAndDescs.emplace(
						make_pair(GetStateFromString(child.Token(1)),
								make_pair(node.Token(1), child.Token(2))));
			}
		}
	}
}



const MilestoneState &Milestone::MilestoneStateFromString(const string &name)
{
	auto &it = stringToMilestoneState.find(name);
	if(it == stringToMilestoneState.end())
		return MilestoneState::DEFAULT;
	return it->second;
}

