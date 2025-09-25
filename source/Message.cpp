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
#include "text/Format.h"
#include "GameData.h"
#include "Phrase.h"
#include "TextReplacements.h"

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
		else if(key == "aggressive deduplication")
			aggressiveDeduplication = true;
		else if(key == "no log deduplication")
			logDeduplication = false;
		else if(key == "important")
			isImportant = true;
		else if(key == "log only")
			logOnly = true;
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



bool Message::Category::AggressiveDeduplication() const
{
	return aggressiveDeduplication;
}



bool Message::Category::LogDeduplication() const
{
	return logDeduplication;
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



const string &Message::Name() const
{
	return name;
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



const Message::Category *Message::GetCategory() const
{
	return category;
}
