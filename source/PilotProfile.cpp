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
#include "Files.h"
#include "GameData.h"
#include "PlayerInfo.h"

using namespace std;

namespace {
	vector<shared_ptr<PilotProfile>> pilots;
}



void PilotProfile::LoadProfiles()
{
	// Clear all existing save file memory from the pilot profiles.
	for(auto &pilot : pilots)
		pilot->Files().clear();

	// Look at all the existing save files and assign them to the appropriate pilot.
	vector<filesystem::path> fileList = Files::List(Files::Saves());
	for(const auto &path : fileList)
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
		if(!pilot->IsLoaded())
			pilot->Load(path);
		auto &savesList = pilot->Files();
		savesList.emplace_back(fileName, Files::Timestamp(path));
		// Ensure that the main save for this pilot, not a snapshot, is first in the list.
		if(!isSnapshot)
			swap(savesList.front(), savesList.back());
	}

	// Discard any profiles that no longer have an associated save file.
	for(auto it = pilots.begin(); it != pilots.end(); )
	{
		shared_ptr<PilotProfile> &profile = *it;
		if(profile->Files().empty())
		{
			// Ensure that the persistent storage file is deleted.
			profile->Delete();
			it = pilots.erase(it);
		}
		else
			++it;
	}

	// Sort the snapshots by timestamp and name. (The main save file was already sorted to the top earlier.)
	for(shared_ptr<PilotProfile> &pilot : pilots)
	{
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



map<string, shared_ptr<PilotProfile>> PilotProfile::GetProfileMap()
{
	map<string, shared_ptr<PilotProfile>> profileMap;
	for(const auto &pilot : pilots)
		profileMap[pilot->Identifier()] = pilot;
	return profileMap;
}



shared_ptr<PilotProfile> &PilotProfile::GetProfile(const string &identifier)
{
	auto it = ranges::find_if(pilots, [&identifier](const shared_ptr<PilotProfile> &pilot) -> bool {
		return pilot->Identifier() == identifier;
	});
	if(it == pilots.end())
		return NewProfile();
	return *it;
}



shared_ptr<PilotProfile> &PilotProfile::NewProfile()
{
	return pilots.emplace_back(make_shared<PilotProfile>());
}



void PilotProfile::New(const Gamerules &gamerules)
{
	this->gamerules.Replace(gamerules);
	GameData::SetGamerules(&this->gamerules);
}



void PilotProfile::Load(const string &path)
{
	if(isLoaded)
		return;
	isLoaded = true;

	SetPath(path);
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



void PilotProfile::Delete()
{
	Files::Delete(filePath);
}



const string &PilotProfile::Path() const
{
	return filePath;
}



void PilotProfile::SetPath(const string &path)
{
	string root = path;
	size_t pos = root.find('~');
	if(pos != string::npos)
		root = root.substr(0, pos) + ".txt";
	root = (root.length() < 4) ? "" : root.substr(0, root.length() - 4);
	if(root.empty())
		return;
	filePath = root + ".ess";
}



string PilotProfile::Identifier() const
{
	string name = Files::Name(filePath);
	return (name.length() < 4) ? "" : name.substr(0, name.length() - 4);
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
