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

#include <map>
#include <string>

class DataNode;
class DataWriter;



// Class containing message data. It's different than the Messages::Entry class,
// which represents entries in the list view.
class Message {
public:
	class Category {
	public:
		enum class DuplicatesStrategy {
			KEEP_NEW,
			KEEP_OLD,
			KEEP_BOTH
		};

	public:
		void Load(const DataNode &node);
		bool IsLoaded() const;

		const std::string &Name() const;
		const Color &MainColor() const;
		const Color &LogColor() const;
		DuplicatesStrategy MainDuplicatesStrategy() const;
		bool AllowsLogDuplicates() const;
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
		DuplicatesStrategy mainDuplicates = DuplicatesStrategy::KEEP_NEW;
		// Avoid duplicating the last log entry.
		bool allowsLogDuplicates = false;
		// Whether to include this category in the message log panel's filter.
		bool isImportant = false;
		// Save this message to the log, but don't show it in the main view.
		bool logOnly = false;
	};


public:
	Message() = default;
	Message(const std::string &text, const Category *category);
	explicit Message(const DataNode &node);
	void Load(const DataNode &node);
	bool IsLoaded() const;
	void Save(DataWriter &out) const;

	const std::string &Name() const;
	bool IsPhrase() const;
	// Choose a message from the phrase if this message has one, or resolve substitutions
	// on the raw text to get the final message string.
	std::string Text() const;
	// Get the final text with custom substitutions, assuming the message is not a phrase.
	std::string Text(const std::map<std::string, std::string> &subs) const;
	const Category *GetCategory() const;


private:
	bool isLoaded = false;
	std::string name;
	// The text, or the name of the phrase used to generate the message.
	std::string text;
	bool isPhrase = false;
	const Category *category = nullptr;
};
