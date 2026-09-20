/* Setting.h
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

#include <functional>
#include <string>

class DataNode;
class UI;



// Represents a settings, such as a preference or gamerule.
// Handles how the setting is displayed and modified.
class Setting {
public:
	// A simple boolean setting with an on and an off value.
	static Setting Boolean(const std::string &name, bool defaultValue);
	// A setting where the player can cycle through a list of options.
	static Setting List(const std::string &name, int defaultValue, const std::vector<std::string> &options,
		bool alwaysOn = false);
	// A setting whose value is handled by Preferences directly.
	static Setting Unique(const std::string &name);


public:
	Setting() = default;
	Setting(const std::string &displayName, const std::vector<std::string> &options, int defaultIndex, bool save,
		bool alwaysOn);

	void SetDisplayFunc(std::function<std::pair<std::string, bool>()> displayFunc);
	void SetToggleFunc(std::function<int(int, UI *)> toggleFunc);
	void SetOnToggleFunc(std::function<void(UI *)> onToggleFunc);
	void SetScrollFunc(std::function<void(double)> scrollFunc);

	const std::string &DisplayName() const;
	// The value to display for this setting, and whether it is "on"/"off" (i.e. should be drawn bright/dim).
	std::pair<std::string, bool> DisplayValue() const;

	void SetIndex(int index);
	int Index() const;
	int Toggle(UI *ui);
	void Scroll(double dy);

	// Update the index to the next option without running the toggle functions.
	int CycleIndex();
	// The string value in the options vector matching the current index.
	const std::string &Option() const;

	bool Save() const;


private:
	std::string displayName;
	// Some settings have a list of options that they can cycle through.
	std::vector<std::string> options;
	int index = 0;
	// Whether this setting should always display as on regardless of the index/value.
	bool alwaysOn = false;
	// Whether this setting's index should be directly saved to the preferences file.
	// If false, Preferences handles saving the value.
	bool save = true;

	// Controls how the value is displayed.
	std::function<std::pair<std::string, bool>()> displayFunc;
	// Controls the toggling of this setting. Takes in the current value and returns the new value.
	std::function<int(int, UI *)> toggleFunc;
	// An additional call that occurs after toggling. Takes in the current value.
	std::function<void(UI *)> onToggleFunc;
	// Controls updating of the setting when scrolling.
	std::function<void(double)> scrollFunc;
};
