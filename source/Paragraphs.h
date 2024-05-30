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

#include "ConditionSet.h"

#include <algorithm>
#include <string>
#include <vector>

class ConditionsStore;
class DataNode;

class Paragraphs {
public:
	// Load one line of text and possible conditions from the given node.
	void Load(const DataNode &node);

	// Discard all description lines.
	void Clear();

	// Is this object totally void of all information?
	bool IsEmpty() const;

	// Are there any lines which match these vars?
	bool IsEmptyFor(const ConditionsStore &vars) const;

	// Concatinate all lines which match these vars.
	std::string ToString(const ConditionsStore &vars) const;

	// Returns a new Paragraphs that contains both this object's lines and the other objects', in that order.
	Paragraphs operator + (const Paragraphs &other) const;

private:
	std::vector<std::pair<ConditionSet, std::string>> text;
};
