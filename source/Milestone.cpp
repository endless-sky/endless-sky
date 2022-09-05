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

using namespace std;

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
		/*if(node.Size() == 1)
		{
			if(node.Token(0) == "hidden")
				isHidden = true;
			else if(node.Token(0) == "locked")
				isLocked = true;
		}*/
		if(node.Token(0) == "hidden")
		{
			isHidden = true;
			continue;
		}
		else if(node.Token(0) == "locked")
			isLocked = true;
		if(node.Token(0) == "to")
		{

		}
		else if(node.Token(0) == "on")
		{

		}
		else if(node.Size() >= 2)
		{
			displayNamesAndDescs.emplace(
					pair<MilestoneState, pair<string, string>>(
							MilestoneState::FromString(node.Token(0)),
							pair<string, string>(
									node.Token(1), node.Token(2))));
			/*if(node.Token(0) == "locked")
			{
				locked.first = node.Token(1);
				locked.second = node.Token(2);
			}
			else if(node.Token(0) == "unlocked")
			{
				unlocked.first = node.Token(1);
				unlocked.second = node.Token(2);
			}
			else if(node.Token(0) == "completed")
			{
				completed.first = node.Token(1);
				completed.second = node.Token(2);
			}*/
		}
	}

}


