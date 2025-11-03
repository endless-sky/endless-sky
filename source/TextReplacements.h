/* TextReplacements.h
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

#pragma once

#include <map>
#include <string>
#include <vector>

class ConditionSet;
class ConditionsStore;
class DataNode;
class PlayerInfo;



// A class containing a list of text replacements. Text replacements consist of a
// key to search for and the text to replace it with. One key can have multiple potential
// replacement texts, with the specific text chosen being defined by whichever replacement
// is the last valid replacement for that key, with replacement validity being defined
// by a ConditionSet.
class TextReplacements {
public:
	// Load a substitutions node.
	void Load(const DataNode &node, const ConditionsStore *playerConditions);

	// Clear this TextReplacement's substitutions and insert the substitutions of other.
	void Revert(TextReplacements &other);

	// Add new text replacements to the given map after evaluating all possible replacements.
	// This TextReplacements will overwrite the value of any existing keys in the given map
	// if the map and this TextReplacements share a key.
	void Substitutions(std::map<std::string, std::string> &subs) const;


private:
	// Vector with "string to be replaced", "condition when to replace", and "replacement text".
	std::vector<std::pair<std::string, std::pair<ConditionSet, std::string>>> substitutions;
};
