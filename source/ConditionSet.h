/* ConditionSet.h
Copyright (c) 2014-2024 by Michael Zahniser and others

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

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

class ConditionsStore;
class DataNode;
class DataWriter;



// A condition set is a collection of operations on the player's set of named "conditions"; "test" operations that just
// check the values of those conditions and "evaluation" operations that can calculate an int64_t value based on the
// conditions.
class ConditionSet {
public:
	ConditionSet() = default;
	ConditionSet(const ConditionSet &) = default;

	// Construct and Load() at the same time.
	explicit ConditionSet(const DataNode &node);

	// Construct a terminal with a literal value.
	explicit ConditionSet(int64_t newLiteral);

	// Load a set of conditions from the children of this node. Prints a
	// warning if the conditions cannot be parsed from the node.
	void Load(const DataNode &node);
	// Save a set of conditions.
	void Save(DataWriter &out) const;
	void SaveChild(int childNr, DataWriter &out) const;
	void SaveSubset(DataWriter &out) const;

	// Change this condition to always be false.
	void MakeNever();

	// Check if there are any entries in this set.
	bool IsEmpty() const;

	// Check if this condition set contains valid data.
	bool IsValid() const;

	// Check if the given condition values satisfy this set of expressions.
	bool Test(const ConditionsStore &conditions) const;

	// Evaluate this expression into a numerical value. (The value can also be used as boolean.)
	int64_t Evaluate(const ConditionsStore &conditionsStore) const;

	/// Parse the remainder of a node into this expression.
	bool ParseNode(const DataNode &node, int &tokenNr);

	/// Optimize this node, this optimization also removes intermediate sections that were used for tracking brackets.
	bool Optimize(const DataNode &node);

	// Get the names of the conditions that are modified by this ConditionSet.
	std::set<std::string> RelevantConditions() const;


public:
	enum class ExpressionOp
	{
		OP_INVALID, ///< Expression is invalid.

		// Direct access operators
		OP_VAR, ///< Direct access to condition variable, no other operations.
		OP_LIT, ///< Direct access to literal, no other operations).

		// Arithmetic operators
		OP_ADD, ///< Adds ( + ) the values from all sub-expressions.
		OP_SUB, ///< Subtracts ( - ) all later sub-expressions from the first one.
		OP_MUL, ///< Multiplies ( * ) all sub-expressions with each-other.
		OP_DIV, ///< (Integer) Divides ( / ) the first sub-expression by all later ones.
		OP_MOD, ///< Modulo ( % ) by the second and later sub-expressions on the first one.

		// Boolean equality operators, return 0 or 1
		OP_EQ, ///< Tests for equality ( == ).
		OP_NE, ///< Tests for not equal to ( != ).
		OP_LE, ///< Tests for less than or equal to ( <= ).
		OP_GE, ///< Tests for greater than or equal to ( >= ).
		OP_LT, ///< Tests for less than ( < ).
		OP_GT, ///< Tests for greater than ( > ).

		// Boolean combination operators, return 0 or 1
		OP_AND, ///< Boolean 'and' operator; returns 0 on first 0 subcondition, value of first sub-condition otherwise.
		OP_OR, ///< Boolean 'or' operator; returns value of first non-zero sub-condition, or zero if all are zero.

		// Single boolean operators
		OP_NOT, ///< Single boolean 'not' operator.
		OP_HAS ///< Single boolean 'has' operator.
	};


private:
	/// Parse a node completely into this expression; all tokens on the line and all children if there are any.
	bool ParseNode(const DataNode &node);


	/// Parse the children under 'and'-nodes, 'or'-nodes, or the toplevel-node (which acts as and-node). The
	/// expression-operator should already have been set before calling this function.
	bool ParseBooleanChildren(const DataNode &node);


	/// Parse a minimal complete expression from the tokens into the (empty) expression.
	///
	/// @param node The node to report parse-errors on, if any occur.
	/// @param lineTokens Tokens to use (and pop from) for parsing.
	bool ParseMini(const DataNode &node, int &tokenNr);


	/// Helper function to parse an infix operator and the subexpression after it.
	/// Should be called on ConditionSets that already have at least 1 sub-expression in them.
	///
	/// @param node The node to report parse-errors on, if any occur.
	/// @param lineTokens Tokens to use (and pop from) for parsing.
	bool ParseFromInfix(const DataNode &node, int &tokenNr, ExpressionOp parentOp);


	/// Replace current node by its first child node.
	bool PromoteFirstChild(const DataNode &node);

	/// Push sub-expressions and the operator from the current expression one level down into a new single
	/// sub-expression.
	///
	/// To be used if the current expressions has precedence over the next infix operator that we are about to parse.
	/// @param node Node on which to report the failures (using node.PrintTrace()).
	bool PushDownFull(const DataNode &node);


	/// Push the last sub-expression from the current expression one level down into a new sub-expression.
	///
	/// To be used when the next infix-operator has precedence over the current operators being processed.
	/// @param node Node on which to report the failures (using node.PrintTrace()).
	bool PushDownLast(const DataNode &node);


	/// Handles a failure in parsing of lower-level nodes, for higher-level nodes;
	/// - Clears the sub-expressions and sets the operator to OP_INVALID.
	bool FailParse();


	/// Handles a failure in parsing;
	/// - Reports the failure using PrintTrace() in the given DataNode.
	/// - Clears the sub-expressions and sets the operator to OP_INVALID.
	///
	/// @param node Node on which to report the failures (using node.PrintTrace()).
	/// @param failText The reason why parsing is failing. (Will be used as output for node.PrintTrace()).
	/// @return false So that is can be used as a one-liner for failures.
	bool FailParse(const DataNode &node, const std::string &failText);


private:
	/// Sets of condition tests can contain nested sets of tests. Each set is
	/// combined using the expression operator that determines how the nested
	/// sets are to be combined.
	/// Using an `and`-operator with no sub-expressions as safe initial value.
	ExpressionOp expressionOperator = OP_AND;
	/// Literal part of the expression, if this is a literal terminal.
	int64_t literal = 0;
	/// Condition variable that is used in this expression, if this is a condition variable.
	std::string conditionName;
	/// Nested sets of conditions to be tested.
	std::vector<ConditionSet> children;

	// Let the assignment class call internal functions and parsers.
	friend class ConditionAssignments;
};
