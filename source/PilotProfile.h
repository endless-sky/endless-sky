/* PilotProfile.h
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

#include "ConditionsStore.h"
#include "Gamerules.h"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class DataWriter;
class PlayerInfo;
class StartConditions;



// Class holding information about a "pilot." A pilot consists of multiple
// save files that are a part of the same run of the game. This allows for
// storage of information across all save files for the same pilot.
class PilotProfile {
public:
	// Read the save file directory to look for all .ess files that contain pilot profiles.
	static void LoadProfiles();
	static std::vector<std::shared_ptr<PilotProfile>> &GetProfiles();
	static std::map<std::string, std::shared_ptr<PilotProfile>> GetProfileMap();
	// Get the profile with the given identifier. If no profile exists, a new one will be made
	// The caller must check IsLoaded and call Load on unloaded profiles to provide them with
	// their file path if they are new.
	static std::shared_ptr<PilotProfile> &GetProfile(const std::string &identifier);
	// Create a new profile for a fresh player. That player must set the file path of the profile
	// after being named.
	static std::shared_ptr<PilotProfile> &NewProfile();


public:
	PilotProfile() = default;
	// Don't allow copying this class.
	PilotProfile(const PilotProfile &) = delete;
	PilotProfile &operator=(const PilotProfile &) = delete;
	PilotProfile(PilotProfile &&) = delete;
	PilotProfile &operator=(PilotProfile &&) = default;
	~PilotProfile() noexcept = default;

	// Make a new pilot.
	void New(const Gamerules &gamerules);
	// Load an existing pilot.
	void Load(const std::filesystem::path &path);
	bool IsLoaded() const;
	// Save this pilot's shared information. (Saving individual save files is done through PlayerInfo.)
	void Save();
	// Delete this pilot's shared information. (Deleting individual save files is done through LoadPanel.)
	void Delete();

	const std::string &Path() const;
	void SetPath(const std::filesystem::path &path);
	std::string Identifier() const;

	// Get the individual save files of this pilot.
	std::vector<std::pair<std::string, std::filesystem::file_time_type>> &Files();
	const std::vector<std::pair<std::string, std::filesystem::file_time_type>> &Files() const;

	// The conditions shared across all save files for this pilot.
	ConditionsStore &Conditions();
	const ConditionsStore &Conditions() const;
	// The gamerules that this pilot plays by.
	Gamerules &GetGamerules();
	const Gamerules &GetGamerules() const;
	void ApplyGamerules() const;


private:
	std::string filePath;
	bool isLoaded = false;

	// The individual save files that make up this pilot.
	std::vector<std::pair<std::string, std::filesystem::file_time_type>> files;

	// The conditions shared across all save files for this pilot.
	ConditionsStore conditions;
	// The gamerules that all save files for this pilot use.
	Gamerules gamerules;
};
