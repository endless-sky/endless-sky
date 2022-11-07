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

#include "DataNode.h"
#include "Track.h"

#include <iostream>


Track::Track(const DataNode &node)
{
	Load(node);
}



void Track::Load(const DataNode &node)
{
	// All tracks need a name.
	if(node.Size() < 2)
	{
		node.PrintTrace("Error: No name specified for track:");
		return;
	}

	if(!name.empty())
	{
		node.PrintTrace("Error: Duplicate definition of track:");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "volume" && child.Size() >= 2)
			volumeModifier = child.Value(1);
		else if (child.Token(0) == "idle" && child.Size() >= 2)
			titles[GameState::IDLE] = child.Token(1);
		else if (child.Token(0) == "combat" && child.Size() >= 2)
			titles[GameState::COMBAT] = child.Token(1);
		else if (child.Token(0) == "landed" && child.Size() >= 2)
			titles[GameState::LANDED] = child.Token(1);
	}
	finishedLoading = true;
}



const std::string Track::GetTitle(GameState state) const
{
	std::cout<<"Hi: "<<finishedLoading<<std::endl;
	if(!finishedLoading)
		return nullptr;
	return titles.find(state)->second;
}



const double Track::GetVolumeModifier() const
{
	return volumeModifier;
}
