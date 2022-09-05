/* Plugins.h
Copyright (c) 2022 by Sam Gleske (samrocketman on GitHub)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PLUGINS_H_
#define PLUGINS_H_

#include <string>


// Tracks enabled and disabled plugins for loading plugin data or skipping it.
// This object is updated by toggling plugins in the Preferences UI.
class Plugins {
public:
	static void Load();
	static void Save();
	static void Freeze();

	static bool Has(const std::string &name);
	static bool HasChanged();
	static bool HasChanged(const std::string &name);
	static bool IsEnabled(const std::string &name);
	static void Set(const std::string &name, bool on = true);
	static void TogglePlugin(const std::string &name);
};



#endif
