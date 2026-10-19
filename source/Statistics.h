/* Statistics.h
Copyright (c) 2026 by Amazinite

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

#include <chrono>
#include <filesystem>

class PilotProfile;
class PlayerInfo;



// A class for tracking statistics about the player.
class Statistics {
public:
	static std::filesystem::path FilePath();


public:
	explicit Statistics(PlayerInfo &player);

	void Load();
	void Save() const;

	void AddPlayTime(std::chrono::nanoseconds timeVal, bool updatePlayer);


private:
	PlayerInfo &player;

	// The amount of in-game time played, in seconds.
	double playTime = 0.;
};
