/* Dialog.cpp
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

#include "Dialog.h"

#include <utility>

#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"
#include "GameData.h"

using namespace std;



Dialog::DialogLine::DialogLine(std::string text)
	: text(std::move(text))
{
}



Dialog::DialogLine::DialogLine(const ExclusiveItem<Phrase> &phrase)
	: phrase(phrase)
{
}



Dialog::DialogLine::DialogLine(const DataNode &node, const ConditionsStore *playerConditions)
{
	const string &key = node.Token(0);
	bool hasValue = node.Size() >= 2;
	if(key == "phrase")
	{
		// Handle named phrases
		//    phrase "A Phrase Name"
		if(hasValue)
			phrase = ExclusiveItem<Phrase>(GameData::Phrases().Get(node.Token(1)));
		else
		{
			// Handle anonymous phrases
			//    phrase
			//       ...
			phrase = ExclusiveItem<Phrase>(Phrase(node));
			// Anonymous phrases do not support "to display"
			return;
		}
	}
	// Handle regular dialog text
	//    "Some thrilling dialog that truly moves the player."
	else
	{
		if(hasValue)
			node.PrintTrace("Ignoring extra tokens.");
		text = key;

		// Prevent a corner case that breaks assumptions. Dialog text cannot be empty (that indicates a phrase).
		if(text.empty())
			text = '\t';
	}

	// Search for "to display" lines.
	for(const DataNode &child : node)
	{
		if(child.Size() != 2 || child.Token(0) != "to" || child.Token(1) != "display" || !child.HasChildren())
			child.PrintTrace("Ignoring unrecognized dialog token");
		else
			condition.Load(child, playerConditions);
	}
}



Dialog::Dialog(const DataNode &node, const ConditionsStore *playerConditions)
{
	Load(node, playerConditions);
}



void Dialog::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	if(node.Size() >= 2)
	{
		const string &value = node.Token(1);
		if(node.Size() == 3 && value == "phrase")
			lines.emplace_back(ExclusiveItem<Phrase>(GameData::Phrases().Get(node.Token(2))));
		else
			lines.emplace_back(value);
	}

	for(const DataNode &child : node)
		lines.emplace_back(child, playerConditions);
}



void Dialog::Save(DataWriter &out) const
{
	// A Dialog being saved has already been instantiated, so all information
	// is stored in the "text" variable.
	out.Write("dialog");
	out.BeginChild();
	{
		// Break the text up into paragraphs.
		for(const string &line : Format::Split(text, "\n\t"))
			out.Write(line);
	}
	out.EndChild();
}



const string &Dialog::Text() const
{
	return text;
}



bool Dialog::IsEmpty() const
{
	return text.empty();
}



void Dialog::Collapse()
{
	if(!text.empty())
		return;

	string resultText;
	for(const DialogLine &line : lines)
	{
		// When checking for a pure-text dialog, reject a dialog with conditions or phrases.
		if(!line.condition.IsEmpty() || line.text.empty())
			return;

		// Concatenated lines should start with a tab and be preceded by end-of-line.
		const string &content = line.text;
		if(!resultText.empty())
		{
			resultText += '\n';
			if(!content.empty() && content[0] != '\t')
				resultText += '\t';
		}
		resultText += content;
	}
	text = resultText;
}



Dialog Dialog::Instantiate(const map<string, string> &subs) const
{
	Dialog result;
	// Result is already cached for dialogs that are pure text at Load() time.
	if(!text.empty())
		result.text = Format::Replace(Phrase::ExpandPhrases(text), subs);
	else
	{
		string resultText;
		for(const DialogLine &line : lines)
		{
			// Skip text that is disabled.
			if(!line.condition.IsEmpty() && !line.condition.Test())
				continue;

			// Evaluate the phrase if we have one, otherwise copy the prepared text.
			string content;
			if(!line.text.empty())
				content = line.text;
			else if(line.phrase.IsStock() && line.phrase->IsEmpty())
				content = "stock phrase";
			else
				content = line.phrase->Get();

			// Expand any ${phrases} and <substitutions>
			content = Format::Replace(Phrase::ExpandPhrases(content), subs);

			// Concatenated lines should start with a tab and be preceded by end-of-line.
			if(!resultText.empty())
			{
				resultText += '\n';
				if(!content.empty() && content[0] != '\t')
					resultText += '\t';
			}
			resultText += content;
		}
		result.text = resultText;
	}

	return result;
}
