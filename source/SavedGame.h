/* SavedGame.h
Copyright (c) 2014 by Michael Zahniser

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

#include <filesystem>
#include <string>

class Sprite;



// This class represents a saved game file. It reads the bare amount of
// information necessary from the file to display it in the "Load Game" panel,
// without doing all the complicated parsing that PlayerInfo does. This is so
// that we only need to have one PlayerInfo instance, and there does not need
// to be logic for copying one PlayerInfo into another.
class SavedGame {
public:
	SavedGame() = default;
	explicit SavedGame(const std::filesystem::path &path);

	void Load(const std::filesystem::path &path);
	const std::filesystem::path &Path() const;
	bool IsLoaded() const;
	void Clear();

	const std::string &Name() const;
	const std::string &Credits() const;
	const std::string &GetDate() const;

	const std::string &GetSystem() const;
	const std::string &GetPlanet() const;
	const std::string &GetPlayTime() const;

	const Sprite *ShipSprite() const;
	const std::string &ShipName() const;


private:
	std::filesystem::path path;

	std::string name;
	std::string credits;
	std::string date;

	std::string system;
	std::string planet;
	std::string playTime;

	const Sprite *shipSprite = nullptr;
	std::string shipName;
};
