/* Playlist.h
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

#pragma once

#include "../../ConditionSet.h"
#include "../../LocationFilter.h"

#include <set>
#include <string>

class DataNode;
class Track;
class PlayerInfo;



/// A grouping of tracks under shared conditions.
class Playlist
{
public:
	void Load(const DataNode &data, const ConditionsStore *conditions, const std::set<const System *> *visitedSystems,
		const std::set<const Planet*> *visitedPlanets);

	bool Matches(const PlayerInfo &player) const;
	const std::set<const Track *> &Tracks() const;
	const std::string &Name() const;


private:
	std::string name;
	std::set<const Track *> tracks;
	ConditionSet toPlay;
	LocationFilter playAt;
};