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
class UI;



// Class holding information about a "pilot." A pilot consists of multiple
// save files that are a part of the same run of the game. This allows for
// storage of information across all save files for the same pilot.
class PilotProfile {
public:
	// Read the profiles and saves directories to construct all known PilotProfiles.
	static void LoadProfiles();
	static std::vector<std::shared_ptr<PilotProfile>> &GetProfiles();
	// Get a map of identifier to pilot profile, optionally excluding pilots with no save files.
	static std::map<std::string, std::shared_ptr<PilotProfile>> GetProfileMap(bool excludeEmpty = true);
	// Get the profile with the given identifier. If no profile exists, a new one will be made
	// The caller should call Load to ensure that the profile is loaded before use.
	static std::shared_ptr<PilotProfile> &GetProfile(const std::string &identifier);
	// Create a new profile for a fresh player. That player must set the identifier of the profile
	// after the player has been named.
	static std::shared_ptr<PilotProfile> &NewProfile();
	// Given a pilot's name, return the identifier that should be used for its save files.
	// If there are multiple pilots with the same name, append a number to the
	// pilot name to generate a unique file name.
	static std::string GetIdentifier(const std::string &pilotName);
	// Delete the given pilot profile, deleting all its associated files and removing it from the
	// list of known profiles. If the UI is provided, inform the player if save file deletion failed.
	static void DeleteProfile(const std::shared_ptr<PilotProfile> &pilot, UI *ui);


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
	// Load an existing pilot. The pilot's identifier must have been set before this can be called,
	// as the identifier is what determines the file path to the pilot profile.
	void Load();
	// Save this pilot's shared information. (Saving individual save files is done through PlayerInfo.)
	void Save();

	const std::string &Path() const;
	std::string Identifier() const;
	void SetIdentifier(const std::string &pilotName);

	// Get the individual save files of this pilot.
	std::vector<std::pair<std::string, std::filesystem::file_time_type>> &Files();
	const std::vector<std::pair<std::string, std::filesystem::file_time_type>> &Files() const;

	// The conditions shared across all save files for this pilot.
	ConditionsStore &Conditions();
	const ConditionsStore &Conditions() const;
	// The gamerules that this pilot plays by.
	Gamerules &GetGamerules();
	const Gamerules &GetGamerules() const;
	// Update GameData to use the gamerules from this pilot.
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
