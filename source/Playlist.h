/* Playlist.h
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

#include "ConditionSet.h"

#include <string>
#include <map>
#include <vector>


class DataNode;


// Class to store a track of music that can be used in a playlist.
class Playlist
{
public:
	Playlist() = default;

	// Construct and Load() at the same time.
	Playlist(const DataNode &node);

	void Load(const DataNode &node);

private:
	std::string name;

	ConditionSet toPlay;
	std::vector<std::string> requiredAttributes;

	int silence = 0;
	// Parameters for generating random silence times:
	int silenceLimit = 0;
	double silenceProb = 0.;
};
