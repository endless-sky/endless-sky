/* Plugin.h
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

#pragma once

#include <filesystem>
#include <set>
#include <string>



// Represents information about a single plugin.
class Plugin {
public:
	class PluginDependencies {
	public:
		// Checks if there are any dependencies of any kind.
		bool IsEmpty() const;
		// Checks if there are any duplicate dependencies. E.g. the same dependency in both required and conflicted.
		bool IsValid() const;

	public:
		// The game version to match against.
		std::string gameVersion;
		// The plugins, if any, which are required by this plugin.
		std::set<std::string> required;
		// The plugins, if any, which are designed to work with this plugin but aren't required.
		std::set<std::string> optional;
		// The plugins, if any, which can't be run alongside this plugin.
		std::set<std::string> conflicted;
	};


public:
	// Checks whether this plugin is valid, i.e. whether it exists.
	bool IsValid() const;
	// Constructs a description of the plugin from its name, tags, dependencies, etc.
	std::string CreateDescription() const;


public:
	// The name that identifies this plugin.
	std::string name;
	// The path to the plugin's folder.
	std::filesystem::path path;
	// The about text, if any, of this plugin.
	std::string aboutText;
	// The version of the plugin as defined by the authors.
	std::string version;

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
};
