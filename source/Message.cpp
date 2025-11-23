/* Message.cpp
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

#include "Message.h"

#include "Command.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"
#include "GameData.h"
#include "Phrase.h"
#include "TextReplacements.h"

#include <cassert>

using namespace std;



void Message::Category::Load(const DataNode &node)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	isLoaded = true;

	auto setColor = [](const DataNode &node, ExclusiveItem<Color> &color) noexcept -> void {
		if(node.Size() >= 4)
			color = ExclusiveItem<Color>{{
				static_cast<float>(node.Value(1)),
				static_cast<float>(node.Value(2)),
				static_cast<float>(node.Value(3))}};
		else
			color = ExclusiveItem<Color>{GameData::Colors().Get(node.Token(1))};
	};

	for(const auto &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "main color" && hasValue)
			setColor(child, mainColor);
		else if(key == "log color" && hasValue)
			setColor(child, logColor);
		else if(key == "main duplicates" && hasValue)
		{
			const string &value = child.Token(1);
			if(value == "keep new")
				mainDuplicates = DuplicatesStrategy::KEEP_NEW;
			else if(value == "keep old")
				mainDuplicates = DuplicatesStrategy::KEEP_OLD;
			else if(value == "keep both")
				mainDuplicates = DuplicatesStrategy::KEEP_BOTH;
		}
		else if(key == "log duplicates" && hasValue)
		{
			const string &value = child.Token(1);
			if(value == "keep old")
				allowsLogDuplicates = false;
			else if(value == "keep both")
				allowsLogDuplicates = true;
		}
		else if(key == "important")
			// By default this doesn't need a value, but support
			// explicit false if a plugin wants to override base data.
			isImportant = !hasValue || child.BoolValue(1);
		else if(key == "log only")
			// By default this doesn't need a value, but support
			// explicit false if a plugin wants to override base data.
			logOnly = !hasValue || child.BoolValue(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool Message::Category::IsLoaded() const
{
	return isLoaded;
}



const string &Message::Category::Name() const
{
	return name;
}



const Color &Message::Category::MainColor() const
{
	return *mainColor;
}



const Color &Message::Category::LogColor() const
{
	return *logColor;
}



Message::Category::DuplicatesStrategy Message::Category::MainDuplicatesStrategy() const
{
	return mainDuplicates;
}



bool Message::Category::AllowsLogDuplicates() const
{
	return allowsLogDuplicates;
}



bool Message::Category::IsImportant() const
{
	return isImportant;
}



bool Message::Category::LogOnly() const
{
	return logOnly;
}



Message::Message(const string &text, const Category *category)
	: text{text}, category{category}
{
}



Message::Message(const DataNode &node)
{
	Load(node);
}



void Message::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	isLoaded = true;

	for(const auto &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "text" && hasValue)
		{
			text = child.Token(1);
			isPhrase = false;
		}
		else if(key == "phrase" && hasValue)
		{
			text = child.Token(1);
			isPhrase = true;
		}
		else if(key == "category" && hasValue)
			category = GameData::MessageCategories().Get(child.Token(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	if(!category)
		category = GameData::MessageCategories().Get("normal");
}



bool Message::IsLoaded() const
{
	return isLoaded;
}



void Message::Save(DataWriter &out) const
{
	// If this message has a name, it's defined globally, so just save a reference.
	if(!name.empty())
	{
		out.Write("message", name);
		return;
	}

	out.Write("message");
	out.BeginChild();
	{
		// If we need to save a customized instance of a message, substitutions
		// should have already been applied, so just write the text.
		out.Write(isPhrase ? "phrase" : "text", text);
		out.Write("category", category->Name());
	}
	out.EndChild();
}



const string &Message::Name() const
{
	return name;
}



bool Message::IsPhrase() const
{
	return isPhrase;
}



string Message::Text() const
{
	if(isPhrase)
		return GameData::Phrases().Get(text)->Get();

	map<string, string> subs;
	GameData::GetTextReplacements().Substitutions(subs);
	for(const auto &[key, value] : subs)
		subs[key] = Phrase::ExpandPhrases(value);
	Format::Expand(subs);
	return Command::ReplaceNamesWithKeys(Format::Replace(Phrase::ExpandPhrases(text), subs));
}



string Message::Text(const map<string, string> &subs) const
{
	assert(!isPhrase && "Cannot apply custom substitutions to a global phrase");
	return Command::ReplaceNamesWithKeys(Format::Replace(Phrase::ExpandPhrases(text), subs));
}



const Message::Category *Message::GetCategory() const
{
	return category;
}
