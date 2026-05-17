/* DialogSettings.cpp
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

#include "DialogSettings.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"
#include "GameData.h"

using namespace std;



DialogSettings::DialogSettings(const DataNode &node, const ConditionsStore *playerConditions)
{
	Load(node, playerConditions);
}



void DialogSettings::Load(const DataNode &node, const ConditionsStore *playerConditions)
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



void DialogSettings::Save(DataWriter &out) const
{
	// A Dialog being saved has already been instantiated, so we only
	// need to store the text of each line, as all phrases have been
	// converted into text.
	out.Write("dialog");
	out.BeginChild();
	{
		for(const DialogLine &line : lines)
		{
			out.Write(line.text);
			out.BeginChild();
			{
				if(!line.condition.IsEmpty())
				{
					out.Write("to", "display");
					out.BeginChild();
					{
						line.condition.Save(out);
					}
					out.EndChild();
				}
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



bool DialogSettings::Validate() const
{
	for(const DialogLine &line : lines)
		if(line.text.empty() && line.phrase.IsStock() && line.phrase->IsEmpty())
			return false;
	return true;
}



string DialogSettings::Text() const
{
	string resultText;
	for(const DialogLine &line : lines)
	{
		// Skip text that is disabled. It's technically possible for all lines of a
		// dialog to be disabled and for this function to return an empty string. This
		// will result in an empty dialog being displayed to the player and be
		// considered a content error.
		if(!line.condition.IsEmpty() && !line.condition.Test())
			continue;

		// Concatenated lines should start with a tab and be preceded by end-of-line.
		const string &text = line.text;
		if(!resultText.empty())
		{
			resultText += '\n';
			if(!text.empty() && text[0] != '\t')
				resultText += '\t';
		}
		resultText += text;
	}

	return resultText;
}



bool DialogSettings::IsEmpty() const
{
	return lines.empty();
}



DialogSettings DialogSettings::Instantiate(const map<string, string> &subs) const
{
	DialogSettings result;
	for(const DialogLine &line : lines)
	{
		// Evaluate the phrase if we have one. Otherwise, copy the text.
		string content = !line.text.empty() ? line.text : line.phrase->Get();

		// Expand any ${phrases} and <substitutions>.
		content = Format::Replace(Phrase::ExpandPhrases(content), subs);

		// Re-store this line with its expanded text and conditions.
		// Conditions are not evaluated until the text is displayed.
		result.lines.emplace_back(content, line.condition);
	}

	return result;
}



DialogSettings::DialogLine::DialogLine(std::string text)
	: text(std::move(text))
{
}



DialogSettings::DialogLine::DialogLine(const ExclusiveItem<Phrase> &phrase)
	: phrase(phrase)
{
}



DialogSettings::DialogLine::DialogLine(const string &text, const ConditionSet &condition)
	: text(text), condition(condition)
{
}



DialogSettings::DialogLine::DialogLine(const DataNode &node, const ConditionsStore *playerConditions)
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
