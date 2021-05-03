/* Preferences.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <string>



class Preferences {
public:
	enum class VSync : int {
		off = 0,
		on,
		adaptive,
	};
	
	
public:
	static void Load();
	static void Save();
	
	static bool Has(const std::string &name);
	static void Set(const std::string &name, bool on = true);
	
	// Toggle the ammo usage preferences, cycling between "never," "frugally,"
	// and "always."
	static void ToggleAmmoUsage();
	static std::string AmmoUsage();
	
	// Scroll speed preference.
	static int ScrollSpeed();
	static void SetScrollSpeed(int speed);
	
	// View zoom.
	static double ViewZoom();
	static bool ZoomViewIn();
	static bool ZoomViewOut();
	
	// VSync setting, either "on", "off", or "adaptive".
	static bool ToggleVSync();
	static Preferences::VSync VSyncState();
	static const std::string &VSyncSetting();
};



#endif
