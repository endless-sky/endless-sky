/* Track.h
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

#include "../../DataNode.h"
#include "Layer.h"

#include <functional>
#include <string>
#include <vector>

class DataNode;



/// A track is a collection of sounds that play together within a playlist.
/// Each track has exactly one file that plays in the background,
/// but can have several feature tracks playing in the foreground.
class Track {
public:
	Track() = default;
	/// A track created for an audio file.
	explicit Track(const std::string &name, double duration = -1.);
	/// A track created with a custom name and data.
	void Load(const DataNode &data);
	/// The name of the track. This is unique for all tracks
	/// except for "silence", which can be duplicated.
	const std::string &Name() const;
	bool Valid() const;

	std::vector<std::reference_wrapper<const Layer>> Layers() const;

private:
	std::string name;
	Layer background;
	std::vector<Layer> foreground;
};
