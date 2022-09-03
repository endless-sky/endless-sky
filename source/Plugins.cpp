/* Plugins.cpp
Copyright (c) 2022 by Sam Gleske (samrocketman on GitHub)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Plugins.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"

#include <algorithm>
#include <map>

using namespace std;

namespace {
	map<string, bool> settings;
	map<string, bool> frozenSettings;

	void LoadSettings(const string &path)
	{
		DataFile prefs(path);
		for(const DataNode &node : prefs)
		{
			const string &key = node.Token(0);
			if(key == "plugins")
				for(const DataNode &child : node)
					if(child.Size() == 2)
						settings[child.Token(0)] = child.Value(1);
		}
	}
}



void Plugins::Load()
{
	// Global plugin settings
	LoadSettings(Files::Resources() + "plugins.txt");
	// Local plugin settings
	LoadSettings(Files::Config() + "plugins.txt");
}



void Plugins::Save()
{
	if(settings.size() == 0)
		return;
	DataWriter out(Files::Config() + "plugins.txt");

	out.Write("plugins");
	out.BeginChild();
	{
		for(const auto &it : settings)
			out.Write(it.first, it.second);
	}
	out.EndChild();

}



// Freeze the plugin set to determine if a setting has changed.
void Plugins::Freeze()
{
	copy(settings.begin(), settings.end(), inserter(frozenSettings, frozenSettings.end()));
}



// Determine if a plugin setting has changed since launching.
bool Plugins::HasChanged(const string &name)
{
	return frozenSettings[name] != settings[name];
}




// Returns true if any plugin enabled or disabled setting has changed since
// launched via user preferences.
bool Plugins::HasChanged()
{
	for(const auto &plugin : frozenSettings)
		if(plugin.second != settings[plugin.first])
			return true;
	return false;
}




// Plugins are enabled by default and disabled if the user prefers it to be
// disabled.
bool Plugins::IsEnabled(const string &name)
{
	auto it = settings.find(name);
	return (it == settings.end()) || it->second;
}



void Plugins::Set(const string &name, bool on)
{
	settings[name] = on;
}



// Toggles enabling or disabling a plugin for the next game restart.
void Plugins::TogglePlugin(const string &name)
{
	settings[name] = !IsEnabled(name);
}
