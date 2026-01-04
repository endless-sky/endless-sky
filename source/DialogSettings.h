/* DialogSettings.h
Copyright (c) 2025 by Amazinite

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

#include "ConditionSet.h"
#include "ExclusiveItem.h"
#include "Phrase.h"

#include <map>
#include <string>
#include <vector>

class ConditionsStore;
class DataNode;
class DataWriter;
class UI;



// A class that parses "dialog" nodes. Dialogs are short texts where the player
// is only able to respond with one or two buttons. Generally used for displaying
// short messages that don't necessitate the use of a full conversation panel.
class DialogSettings {
public:
	DialogSettings() = default;
	// Construct and Load() at the same time.
	DialogSettings(const DataNode &node, const ConditionsStore *playerConditions);

	void Load(const DataNode &node, const ConditionsStore *playerConditions);
	void Save(DataWriter &out) const;

	bool Validate() const;

	// Apply any replacements, evaluate any condition sets, and generate from any phrases.
	DialogSettings Instantiate(const std::map<std::string, std::string> &subs) const;

	// Get the text of this dialog (after it has been instantiated and converted into a single block of text).
	const std::string &Text() const;
	bool IsEmpty() const;


private:
	// For individual lines under the dialog.
	class DialogLine {
	public:
		explicit DialogLine(std::string text);
		explicit DialogLine(const ExclusiveItem<Phrase> &phrase);
		DialogLine(const DataNode &node, const ConditionsStore *playerConditions);

		std::string text;
		ConditionSet condition;
		ExclusiveItem<Phrase> phrase;
	};


private:
	// Lines of text under the `dialog` node that haven't yet been instantiated into a single paragraph.
	std::vector<DialogLine> lines;
	// The instantiated string from the dialog lines, with all text substitions applied and phrases expanded.
	std::string text;
};
