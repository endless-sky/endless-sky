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
#include <vector>

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


public:
	/// Possible assignment operators.
	enum AssignOp
	{
		AO_ASSIGN, /// Used for =, set (with 1 as expression), clear ((with 0 as expression)
		AO_ADD, /// Used for +=, ++ (with 1 as expression)
		AO_SUB, /// Used for -=, -- (with 1 as expression)
		AO_MUL, /// Used for *=
		AO_DIV, /// Used for /= (integer division)
		AO_LT,  /// Used for <?=
		AO_GT  /// Used for >?=
	};


private:
	// Class that supports a single assignment
	class Assignment {

	public:
		std::string conditionToAssignTo;
		AssignOp assignOperator;
		ConditionSet expressionToEvaluate;
	};


private:
	std::vector<Assignment> assignments;
};
