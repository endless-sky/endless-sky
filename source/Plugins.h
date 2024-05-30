/* Plugins.h
Copyright (c) 2022 by Sam Gleske (samrocketman on GitHub)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PLUGINS_H_
#define PLUGINS_H_

#include "Set.h"

#include <future>
#include <set>
#include <string>



// Represents information about a single plugin.
struct Plugin {
	struct PluginDependencies {
		// Checks if there are any dependencies of any kind.
		bool IsEmpty() const;
		// Checks if there are any duplicate dependencies. E.g. the same dependency in both required and conflicted.
		bool IsValid() const;

		// The game version to match against.
		std::string gameVersion;
		// The plugins, if any, which are required by this plugin.
		std::set<std::string> required;
		// The plugins, if any, which are designed to work with this plugin but aren't required.
		std::set<std::string> optional;
		// The plugins, if any, which can't be run along side this plugin.
		std::set<std::string> conflicted;
	};

	// Checks whether this plugin is valid, i.e. whether it exists.
	bool IsValid() const;
	// Constructs a description of the plugin from its name, tags, dependencies, etc.
	std::string CreateDescription() const;

	// The name that identifies this plugin.
	std::string name;
	// The path to the plugin's folder.
	std::string path;
	// The about text, if any, of this plugin.
	std::string aboutText;
	// The version of this plugin, importanted if it has been installed over ES.
	std::string version = "???";

	// The set of tags which are used to categorize the plugin.
	std::set<std::string> tags;
	// The set of people who have created the plugin.
	std::set<std::string> authors;

	// Other plugins which are required for, optional for, or conflict with this plugin.
	PluginDependencies dependencies;

	// Whether this plugin was enabled, i.e. if it was loaded by the game.
	bool enabled = true;
	// The current state of the plugin.
	bool currentState = true;
	// If this plugin has been deleted.
	bool removed = false;
};



// Tracks enabled and disabled plugins for loading plugin data or skipping it.
// This object is updated by toggling plugins in the Preferences UI.
class Plugins {
public:

	struct InstallData
	{
		std::string name;
		std::string url;
		std::string version;
		std::string aboutText;
		bool installed = false;
		bool outdated = false;
		InstallData(std::string name = "", std::string url = "", std::string version = "",
			std::string aboutText = "", bool installed = false, bool outdated = false)
		: name(name), url(url), version(version), aboutText(aboutText), installed(installed),
			outdated(outdated) {}
	};


public:
	// Attempt to load a plugin at the given path.
	static const Plugin *Load(const std::string &path);

	static void LoadSettings();
	static void Save();

	// Whether the path points to a valid plugin.
	static bool IsPlugin(const std::string &path);
	// Returns true if any plugin enabled or disabled setting has changed since
	// launched via user preferences.
	static bool HasChanged();
	// Returns true if there is active install/update activity;
	static bool IsInBackground();

	// Returns the list of plugins that have been identified by the game.
	static const Set<Plugin> &Get();

	// Toggles enabling or disabling a plugin for the next game restart.
	static void TogglePlugin(const std::string &name);

	// Install or update or delete a plugin.
	static std::future<void> Install(InstallData *installData, bool update = false);
	static std::future<void> Update(InstallData *installData);
	static void DeletePlugin(const std::string &pluginName);

	static bool Download(const std::string &url, const std::string &location);
};



#endif
