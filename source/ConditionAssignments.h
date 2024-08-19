/* ConditionAssignments.h
Copyright (c) 2024 by Peter van der Meer

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

#include <set>
#include <string>

class ConditionsStore;
class DataNode;
class DataWriter;



// Condition assignments is a collection of assignment operations that can be applied on the player's set of named
// "conditions" to modify them.
class ConditionAssignments {
public:
	ConditionAssignments() = default;

	// Construct and Load() at the same time.
	explicit ConditionAssignments(const DataNode &node);

	// Load a set of assignment expressions from the children of this node.
	void Load(const DataNode &node);

	// Save a set of assignment expressions.
	void Save(DataWriter &out) const;

	// Check if there are any entries in this set.
	bool IsEmpty() const;

	// Modify the given set of conditions with the assignments in this class.
	// Order of operations is the order of specification: assignments are applied in the order given.
	void Apply(ConditionsStore &conditions) const;

	// Get the names of the conditions that are modified by this ConditionSet.
	std::set<std::string> RelevantConditions() const;

	// Add an extra assignment to set a condition.
	void AddSetCondition(const std::string &name);

	// Add an extra condition assignment from a data node.
	void Add(const DataNode &node);

private:
	ConditionSet setToEvaluate;
};
