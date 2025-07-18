/* Message.h
Copyright (c) 2025 by TomGoodIdea

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

#include "Color.h"
#include "ExclusiveItem.h"

#include <string>

class DataNode;



// Class containing message data. It's different than the Messages::Entry class,
// which represents entries in the list view.
class Message {
public:
	class Category {
	public:
		void Load(const DataNode &node);
		bool IsLoaded() const;

		const std::string &Name() const;
		const Color &MainColor() const;
		const Color &LogColor() const;
		bool AggressiveDeduplication() const;
		bool LogDeduplication() const;
		bool IsImportant() const;
		bool LogOnly() const;

	private:
		bool isLoaded = false;
		std::string name;
		// The color used in the main panel.
		ExclusiveItem<Color> mainColor;
		// The color used in the message log panel.
		ExclusiveItem<Color> logColor;
		// Avoid duplicates in the list on the main panel.
		bool aggressiveDeduplication = false;
		// Avoid duplicating the last log entry.
		bool logDeduplication = true;
		// Whether to include this category in the message log panel's filter.
		bool isImportant = false;
		// Save this message to the log, but don't show it in the main view.
		bool logOnly = false;
	};


public:
	Message() = default;
	Message(const std::string &text, const Category *category);
	void Load(const DataNode &node);
	bool IsLoaded() const;

	const std::string &Name() const;
	std::string Text() const;
	const Category *GetCategory() const;


private:
	bool isLoaded = false;
	std::string name;
	// The text, or the name of the phrase used to generate the message.
	std::string text;
	bool isPhrase = false;
	const Category *category = nullptr;
};
