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

#include <string>



// Represents information about a single plugin.
struct Plugin {
	// Checks whether this plugin is valid, i.e. whether it exists.
	bool IsValid() const;

	// The name that identifies this plugin.
	std::string name;
	// The path to the plugin's folder.
	std::string path;
	// The about text, if any, of this plugin.
	std::string aboutText;

	// Whether this plugin was enabled, i.e. if it was loaded by the game.
	bool enabled = true;
	// The current state state of the plugin.
	bool currentState = true;
};



// Tracks enabled and disabled plugins for loading plugin data or skipping it.
// This object is updated by toggling plugins in the Preferences UI.
class Plugins {
public:
	// Load a plugin at the given path.
	static const Plugin *Load(const std::string &path);

	static void LoadSettings();
	static void Save();

	// Whether the path points to a valid plugin.
	static bool IsPlugin(const std::string &path);
	// Returns true if any plugin enabled or disabled setting has changed since
	// launched via user preferences.
	static bool HasChanged();

	// Returns the list of plugins that have been identified by the game.
	static const Set<Plugin> &Get();

	// Toggles enabling or disabling a plugin for the next game restart.
	static void TogglePlugin(const std::string &name);
};



#endif
