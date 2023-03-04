/* Track.h
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

#ifndef TRACK_H
#define TRACK_H

#include <map>
#include <string>

class DataNode;



// Class to store a track of music that can be used in a playlist.
class Track {
public:
	enum class GameState
	{
		IDLE,
		COMBAT,
		LANDED
	};
public:
	Track() = default;

	// Construct and Load() at the same time.
	Track(const DataNode &node);

	void Load(const DataNode &node);

	const std::string Name() const;

	const std::string GetTitle(GameState state) const;
	const double GetVolumeModifier() const;
	const int Wait() const;

private:
	std::string name;
	double volumeModifier = 0.;
	std::string idleTitle;
	std::string combatTitle;
	std::string landedTitle;

	int wait = 0;

	bool finishedLoading = false;
};



#endif
