/* GameVersionConstraints.h
Copyright (c) 2025 by TomGoodIdea

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

#include "GameVersion.h"

#include <string>

class DataNode;



// A helper class for version comparisons.
class GameVersionConstraints {
public:
	// A default constraint is undefined, matches every version,
	// and can be used in places that require high compatibility, such as player saves.
	constexpr GameVersionConstraints() = default;
	constexpr GameVersionConstraints(const GameVersion &min, const GameVersion &max);
	// Construct a constraint accepting only the given version.
	explicit constexpr GameVersionConstraints(const GameVersion &singleVersion);
	// Load from plugin metadata.
	void Load(const DataNode &node);

	bool IsEmpty() const;
	// Check if the given version is within all defined bounds.
	bool Matches(const GameVersion &compare) const;

	// A description in the format used by the plugin panel.
	std::string Description() const;


private:
	GameVersion min;
	GameVersion max;
};



constexpr GameVersionConstraints::GameVersionConstraints(const GameVersion &min, const GameVersion &max)
	: min{min}, max{max}
{
}



constexpr GameVersionConstraints::GameVersionConstraints(const GameVersion &singleVersion)
	: GameVersionConstraints{singleVersion, singleVersion}
{
}
