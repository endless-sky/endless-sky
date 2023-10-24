/* TextReplacements.cpp
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "TextReplacements.h"

#include "ConditionSet.h"
#include "ConditionsStore.h"
#include "DataNode.h"

#include <set>

using namespace std;



// Load a substitutions node.
void TextReplacements::Load(const DataNode &node)
{
	// Check for reserved keys. Only some hardcoded replacement keys are
	// reserved, as these ones are done on the fly after all other replacements
	// have been done.
	const set<string> reserved = {"<first>", "<last>", "<ship>"};

	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Skipping substitution key with no replacement:");
			continue;
		}

		string key = child.Token(0);
		if(key.empty())
		{
			child.PrintTrace("Error: Cannot replace the empty string:");
			continue;
		}
		if(key.front() != '<')
		{
			key = "<" + key;
			child.PrintTrace("Warning: text replacements must be prefixed by \"<\":");
		}
		if(key.back() != '>')
		{
			key += ">";
			child.PrintTrace("Warning: text replacements must be suffixed by \">\":");
		}
		if(reserved.count(key))
		{
			child.PrintTrace("Skipping reserved substitution key:");
			continue;
		}

		ConditionSet toSubstitute(child);
		substitutions.emplace_back(key, make_pair(std::move(toSubstitute), child.Token(1)));
	}
}



// Clear this TextReplacement's substitutions and insert the substitutions of other.
void TextReplacements::Revert(TextReplacements &other)
{
	substitutions.clear();
	substitutions.insert(substitutions.begin(), other.substitutions.begin(), other.substitutions.end());
}



// Add new text replacements to the given map after evaluating all possible replacements.
// This text replacement will overwrite the value of any existing keys in the given map
// if the map and this TextReplacements share a key.
void TextReplacements::Substitutions(map<string, string> &subs, const ConditionsStore &conditions) const
{
	for(const auto &sub : substitutions)
	{
		const string &key = sub.first;
		const ConditionSet &toSub = sub.second.first;
		const string &replacement = sub.second.second;
		if(toSub.Test(conditions))
			subs[key] = replacement;
	}
}
