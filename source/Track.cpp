/* Track.cpp
Copyright (c) 2022 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Track.h"

#include "DataNode.h"

#include <algorithm>
#include <iostream>

using namespace std;



Track::Track(const DataNode &node)
{
	Load(node);
}



void Track::Load(const DataNode &node)
{
	if(!name.empty())
	{
		node.PrintTrace("Error: Duplicate definition of track:");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "volume" && hasValue)
			volumeModifier = clamp<double>(child.Value(1), -1., 1.);
		else if(key == "idle" && hasValue)
			idleTitle = child.Token(1);
		else if(key == "combat" && hasValue)
			combatTitle = child.Token(1);
		else if(key == "landed" && hasValue)
			landedTitle = child.Token(1);
		else if(key == "wait" && hasValue)
			wait = max<int>(0, child.Value(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const string &Track::Name() const
{
	return name;
}



const string &Track::GetTitle(GameState state) const
{
	switch(state)
	{
		case GameState::COMBAT:
			return combatTitle;
		case GameState::LANDED:
			return landedTitle;
		case GameState::IDLE:
		default:
			return idleTitle;
	}
}



double Track::GetVolumeModifier() const
{
	return volumeModifier;
}



int Track::Wait() const
{
	return wait;
}
