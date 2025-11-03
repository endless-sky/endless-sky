/* Paragraphs.h
Copyright (c) 2024 by an anonymous author

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

#include <string>
#include <utility>
#include <vector>

class ConditionsStore;
class DataNode;



// Stores a list of description paragraphs, and a condition under which each should be shown.
// See the planet and spaceport description code for examples.
class Paragraphs {
public:
	using ConditionalText = std::vector<std::pair<ConditionSet, std::string>>;
	using ConstIterator = ConditionalText::const_iterator;


public:
	// Load one line of text and possible conditions from the given node.
	void Load(const DataNode &node, const ConditionsStore *playerConditions);

	// Discard all description lines.
	void Clear();

	// Is this object totally void of all information?
	bool IsEmpty() const;

	// Are there any lines which match these vars?
	bool IsEmptyFor() const;

	// Concatenate all lines which match these vars.
	std::string ToString() const;

	// Iterate over all text. Needed to support PrintData.
	// These must use standard naming conventions (begin, end) for compatibility with range-based for loops.
	ConstIterator begin() const;
	ConstIterator end() const;


private:
	ConditionalText text;
};
