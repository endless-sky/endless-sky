/* Playlist.cpp
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

#include "Playlist.h"

#include "../DataNode.h"
#include "../GameData.h"
#include "../PlayerInfo.h"
#include "../System.h"

#include <algorithm>

using namespace std;

namespace {
	const Track *currentTrack = nullptr;
}



Playlist::Playlist(const DataNode &node)
{
	Load(node);
}



void Playlist::Load(const DataNode &node)
{
	if(!name.empty())
	{
		node.PrintTrace("Error: Duplicate definition of playlist:");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "to" && child.Token(1) == "play")
			toPlay.Load(child);
		else if(key == "location")
			location.Load(child);
		else if(key == "priority" && hasValue)
		{
			priority = max<unsigned>(0, child.Value(1));
			if(child.Value(1) < 0)
				child.PrintTrace("Warning: Negative weight specified for priority.");
		}
		else if(key == "weight" && hasValue)
		{
			weight = max<unsigned>(1, child.Value(1));
			if(child.Value(1) < 0)
				child.PrintTrace("Warning: Negative weight specified for priority.");
		}
		else if(key == "tracks")
		{
			if(hasValue)
			{
				if(child.Token(1) == "linear")
					progressionStyle = ProgressionStyle::LINEAR;
				else if(child.Token(1) == "pick")
					progressionStyle = ProgressionStyle::PICK;
				else if(child.Token(1) == "random")
					progressionStyle = ProgressionStyle::RANDOM;
				else
				{
					if(hasValue)
						child.PrintTrace("Warning: \"" + child.Token(1) + "\" is not a valid progression style so using linear:");
					progressionStyle = ProgressionStyle::LINEAR;
				}
			}
			for(const auto &grand : child)
			{
				int trackWeight = (grand.Size() >= 2) ? max<int>(1, grand.Value(1)) : 1;
				tracks.emplace_back(trackWeight, GameData::Tracks().Get(grand.Token(0)));
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Playlist::Activate() const
{
	// Linear should always get the first track in the list when activating.
	if(progressionStyle == ProgressionStyle::LINEAR && tracks.size() > 0)
		currentTrack = tracks.front();
	else
		currentTrack = tracks.Get();
}



// Get the next track as defined by progression style.
const Track *Playlist::GetNextTrack() const
{
	switch(progressionStyle)
	{
		case ProgressionStyle::LINEAR:
		{
			const Track *tmpTrack = currentTrack;
			auto it = find(tracks.begin(), tracks.end(), tmpTrack);
			++it;
			if(it == tracks.end())
				it = tracks.begin();
			currentTrack = *it;
			return tmpTrack;
		}
		case ProgressionStyle::PICK:
			return currentTrack;
		case ProgressionStyle::RANDOM:
		default:
			return tracks.Get();
	}
}



bool Playlist::MatchesConditions(const PlayerInfo &player) const
{
	if(player.GetPlanet() && !location.Matches(player.GetPlanet()))
		return false;
	return toPlay.Test(player.Conditions()) && location.Matches(player.GetSystem());
}



unsigned Playlist::Priority() const
{
	return priority;
}



unsigned Playlist::Weight() const
{
	return weight;
}
