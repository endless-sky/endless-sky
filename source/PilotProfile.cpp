/* PilotProfile.cpp
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

#include "PilotProfile.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "DialogPanel.h"
#include "Files.h"
#include "GameData.h"
#include "PlayerInfo.h"
#include "UI.h"

#include <cassert>
#include <ranges>

using namespace std;

namespace {
	vector<shared_ptr<PilotProfile>> pilots;
}



void PilotProfile::LoadProfiles()
{
	// Clear all existing save file memory from the pilot profiles.
	for(auto &pilot : pilots)
		pilot->Files().clear();

	// Look for all existing pilot profiles.
	for(const filesystem::path &path : Files::List(Files::Profiles()))
	{
		// Skip any files that aren't text files.
		if(path.extension() != ".txt")
			continue;

		string pilotName = Files::NameNoExtension(path);
		GetProfile(pilotName)->Load();
	}

	// Look at all the existing save files and assign them to the appropriate pilot.
	for(const filesystem::path &path : Files::List(Files::Saves()))
	{
		// Skip any files that aren't text files.
		if(path.extension() != ".txt")
			continue;

		string fileName = Files::Name(path);
		// The file name is either "Pilot Name.txt" or "Pilot Name~SnapshotTitle.txt".
		size_t pos = fileName.find('~');
		const bool isSnapshot = (pos != string::npos);
		if(!isSnapshot)
			pos = fileName.size() - 4;

		string pilotName = fileName.substr(0, pos);
		shared_ptr<PilotProfile> &pilot = GetProfile(pilotName);
		// If this pilot profile hasn't been seen before, load it.
		// We need to do this again here since save files may exist that
		// didn't have a corresponding pilot file, in which case GetProfile
		// will return a new pilot.
		pilot->Load();
		auto &savesList = pilot->Files();
		savesList.emplace_back(fileName, Files::Timestamp(path));
		// Ensure that the main save for this pilot, not a snapshot, is first in the list.
		if(!isSnapshot)
			swap(savesList.front(), savesList.back());
	}

	// Sort the snapshots by timestamp and name. (The main save file was already sorted to the top earlier.)
	for(shared_ptr<PilotProfile> &pilot : pilots)
	{
		// Skip pilots with no save files.
		if(pilot->Files().empty())
			continue;
		// Don't include the first item in the sort if this pilot has a non-snapshot save.
		auto start = pilot->Files().begin();
		if(start->first.find('~') == string::npos)
			++start;
		sort(start, pilot->Files().end(),
			[](const pair<string, filesystem::file_time_type> &a, const pair<string, filesystem::file_time_type> &b)
				-> bool
			{
				return a.second > b.second || (a.second == b.second && a.first < b.first);
			}
		);
	}
}



vector<shared_ptr<PilotProfile> > &PilotProfile::GetProfiles()
{
	return pilots;
}



map<string, shared_ptr<PilotProfile>> PilotProfile::GetProfileMap(bool excludeEmpty)
{
	map<string, shared_ptr<PilotProfile>> profileMap;
	for(const auto &pilot : pilots)
	{
		if(excludeEmpty && pilot->Files().empty())
			continue;
		profileMap[pilot->Identifier()] = pilot;
	}
	return profileMap;
}



shared_ptr<PilotProfile> &PilotProfile::GetProfile(const string &identifier)
{
	auto it = ranges::find_if(pilots, [&identifier](const shared_ptr<PilotProfile> &pilot) -> bool {
		return pilot->Identifier() == identifier;
	});
	if(it == pilots.end())
	{
		shared_ptr<PilotProfile> &pilot = NewProfile();
		pilot->SetIdentifier(identifier);
		return pilot;
	}
	return *it;
}



shared_ptr<PilotProfile> &PilotProfile::NewProfile()
{
	return pilots.emplace_back(make_shared<PilotProfile>());
}



std::string PilotProfile::GetIdentifier(const std::string &pilotName)
{
	string name = pilotName;
	string basePath = (Files::Profiles() / name).string();
	int index = 0;
	while(true)
	{
		string path = basePath;
		if(index++)
		{
			string suffix = " " + to_string(index);
			path += suffix;
			name = pilotName + suffix;
		}
		path += ".txt";

		if(!Files::Exists(path))
			break;
	}

	return name;
}



void PilotProfile::DeleteProfile(const std::shared_ptr<PilotProfile> &pilot, UI *ui)
{
	bool failed = false;
	for(const std::string &file : pilot->Files() | views::keys)
	{
		filesystem::path path = Files::Saves() / file;
		Files::Delete(path);
		failed |= Files::Exists(path);
	}
	Files::Delete(pilot->Path());
	failed |= Files::Exists(pilot->Path());
	if(!failed)
	{
		auto it = ranges::find(pilots, pilot);
		if(it != pilots.end())
			pilots.erase(it);
	}
	else if(ui)
		ui->Push(DialogPanel::Info("Deleting pilot files failed."));
}



void PilotProfile::New(const Gamerules &gamerules)
{
	this->gamerules.Replace(gamerules);
	ApplyGamerules();
}



void PilotProfile::Load()
{
	if(isLoaded)
		return;
	isLoaded = true;
	assert(filePath.empty() && "should call SetIdentifier before calling Load");

	DataFile file(filePath);
	for(const DataNode &child : file)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "conditions")
			conditions.Load(child);
		else if(key == "gamerules" && hasValue)
		{
			const string &presetName = child.Token(1);
			const Gamerules *preset = GameData::GamerulesPresets().Find(presetName);
			if(!preset)
			{
				child.PrintTrace("The gamerule preset \"" + presetName + "\" does not exist. "
					"Falling back to the default gamerules.");
				preset = &GameData::DefaultGamerules();
			}
			// Set the player's gamerules to be an exact copy of the selected preset,
			// then load any stored customizations on top of that.
			gamerules.Replace(*preset);
			if(child.HasChildren())
				gamerules.Load(child);
		}
	}

	// If the pilot's gamerules were never loaded, then this is an
	// old pilot that was implicitly using the default gamerules before.
	if(gamerules.Name().empty())
	{
		gamerules.Replace(GameData::DefaultGamerules());
		// Unlock the gamerules for old pilots that never had the option
		// to have them unlocked in the first place.
		gamerules.SetLockGamerules(false);
	}
}



bool PilotProfile::IsLoaded() const
{
	return isLoaded;
}



void PilotProfile::Save()
{
	if(filePath.empty())
		return;
	DataWriter out(filePath);

	conditions.Save(out);

	// Only save gamerules that were customized to be different from the defaults of the chosen preset.
	// If the chosen preset that the gamerules refer to doesn't exist, compare against the default
	// gamerules. A warning will have already been printed for this when the save file was loaded.
	const Gamerules *preset = GameData::GamerulesPresets().Find(gamerules.Name());
	if(!preset)
		preset = &GameData::DefaultGamerules();
	gamerules.Save(out, *preset);
}



const string &PilotProfile::Path() const
{
	return filePath;
}



string PilotProfile::Identifier() const
{
	return Files::NameNoExtension(filePath);
}



void PilotProfile::SetIdentifier(const string &pilotName)
{
	filePath = (Files::Profiles() / (pilotName + ".txt")).string();
}



vector<pair<string, filesystem::file_time_type>> &PilotProfile::Files()
{
	return files;
}



const vector<pair<string, filesystem::file_time_type>> &PilotProfile::Files() const
{
	return files;
}



ConditionsStore &PilotProfile::Conditions()
{
	return conditions;
}



const ConditionsStore &PilotProfile::Conditions() const
{
	return conditions;
}



Gamerules &PilotProfile::GetGamerules()
{
	return gamerules;
}



const Gamerules &PilotProfile::GetGamerules() const
{
	return gamerules;
}



void PilotProfile::ApplyGamerules() const
{
	GameData::SetGamerules(&gamerules);
}
