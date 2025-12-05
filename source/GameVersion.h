/* GameVersion.h
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

#include <array>
#include <string>



// A class representing a version of the game.
class GameVersion {
public:
	static consteval GameVersion Running();


public:
	constexpr GameVersion(unsigned major, unsigned minor, unsigned release,
		unsigned patch = 0, bool fullRelease = true);

	std::string ToString() const;


private:
	std::array<unsigned, 4> numbers{};
	bool fullRelease = true;
};



consteval GameVersion GameVersion::Running()
{
	return {0, 10, 17, 0, false};
}



constexpr GameVersion::GameVersion(unsigned major, unsigned minor, unsigned release, unsigned patch, bool fullRelease)
	: numbers{major, minor, release, patch}, fullRelease{fullRelease}
{
}
