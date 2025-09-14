/* Playlist.cpp
Copyright (c) 2025 by tibetiroka

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

#include "../../DataNode.h"
#include "../../GameData.h"
#include "../../Logger.h"
#include "../../PlayerInfo.h"
#include "../../UniverseObjects.h"

using namespace std;



void Playlist::Load(const DataNode &data, const ConditionsStore *conditions, const set<const System *> *visitedSystems,
	const set<const Planet *> *visitedPlanets)
{
	if(data.Size() > 1)
		name = data.Token(1);
	else
		Logger::LogError("Playlists must have a name");
	for(const DataNode &child : data)
	{
		if(child.Token(0) == "tracks")
		{
			for(const DataNode &trackNode : child)
			{
				const Track *track;
				if(trackNode.Size() > 1)
					track = GameData::GetOrCreateTrack(trackNode.Token(0), trackNode.Value(1));
				else
					track = GameData::GetOrCreateTrack(trackNode.Token(0));
				tracks.emplace(track);
			}
		}
		else if(child.Token(0) == "to play")
			toPlay.Load(child, conditions);
		else if(child.Token(0) == "play at")
			playAt.Load(child, visitedSystems, visitedPlanets);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool Playlist::Matches(const PlayerInfo &player) const
{
	if(!toPlay.Evaluate())
		return false;
	LocationFilter filter = playAt.SetOrigin(player.GetSystem());
	bool value;
	if(player.Flagship())
		value = filter.Matches(*player.Flagship());
	else
		value = filter.Matches(player.GetSystem());
	return value;
}



const set<const Track*> &Playlist::Tracks() const
{
	return tracks;
}



const string &Playlist::Name() const
{
	return name;
}
