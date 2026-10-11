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
	/// Possible assignment operators.
	enum class AssignOp {
		ASSIGN, /// Used for =, set (with 1 as expression), clear ((with 0 as expression)
		ADD, /// Used for +=, ++ (with 1 as expression)
		SUB, /// Used for -=, -- (with 1 as expression)
		MUL, /// Used for *=
		DIV, /// Used for /= (integer division)
		LT,  /// Used for <?=
		GT  /// Used for >?=
	};


public:
	ConditionAssignments() = default;

	// Construct and Load() at the same time.
	explicit ConditionAssignments(const DataNode &node, const ConditionsStore *conditions);

	// Load a set of assignment expressions from the children of this node.
	void Load(const DataNode &node, const ConditionsStore *conditions);

	// Save a set of assignment expressions.
	void Save(DataWriter &out) const;

	// Check if there are any entries in this set.
	bool IsEmpty() const;

	// Modify the conditions with the assignments in this class.
	// Order of operations is the order of specification: assignments are applied in the order given.
	void Apply() const;

	// Get the names of the conditions that are modified by this ConditionSet.
	std::set<std::string> RelevantConditions() const;

	// Add an extra assignment to set a condition.
	void AddSetCondition(const std::string &name, const ConditionsStore *conditions);

	// Add an extra condition assignment from a data node.
	void Add(const DataNode &node, const ConditionsStore *conditions);


private:
	// Class that supports a single assignment
	class Assignment {
	public:
		Assignment(std::string conditionToAssignTo, AssignOp assignOperator, ConditionSet expressionToEvaluate);


	public:
		std::string conditionToAssignTo;
		AssignOp assignOperator;
		ConditionSet expressionToEvaluate;
	};


private:
	// A pointer to the ConditionsStore that this assignment is applied to.
	const ConditionsStore *conditions;

	std::vector<Assignment> assignments;
};
