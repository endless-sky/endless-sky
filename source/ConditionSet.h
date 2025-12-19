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
	enum class ExpressionOp {
		INVALID, ///< Expression is invalid.

		// Direct access operators
		VAR, ///< Direct access to condition variable, no other operations.
		LIT, ///< Direct access to literal, no other operations).

		// Arithmetic operators
		ADD, ///< Adds ( + ) the values from all sub-expressions.
		SUB, ///< Subtracts ( - ) all later sub-expressions from the first one.
		MUL, ///< Multiplies ( * ) all sub-expressions with each-other.
		DIV, ///< (Integer) Divides ( / ) the first sub-expression by all later ones.
		MOD, ///< Modulo ( % ) by the second and later sub-expressions on the first one.
		MIN, ///< 'min' operator, returns the minimum value over all children.
		MAX, ///< 'max' operator, returns the maximum value over all children.

		// Boolean equality operators, return 0 or 1
		EQ, ///< Tests for equality ( == ).
		NE, ///< Tests for not equal to ( != ).
		LE, ///< Tests for less than or equal to ( <= ).
		GE, ///< Tests for greater than or equal to ( >= ).
		LT, ///< Tests for less than ( < ).
		GT, ///< Tests for greater than ( > ).

		// Boolean combination operators, return 0 or 1
		AND, ///< Boolean 'and' operator; returns 0 on first 0 subcondition, value of first sub-condition otherwise.
		OR, ///< Boolean 'or' operator; returns value of first non-zero sub-condition, or zero if all are zero.
	};


public:
	ConditionSet() = default;
	ConditionSet(const ConditionSet &) = default;

	ConditionSet &operator=(const ConditionSet &&other) noexcept;
	ConditionSet &operator=(const ConditionSet &other);

	// Construct an empty set with a pointer to a ConditionsStore.
	explicit ConditionSet(const ConditionsStore *conditions);
	// Construct and Load() at the same time.
	explicit ConditionSet(const DataNode &node, const ConditionsStore *conditions);

	// Construct a terminal with a literal value.
	explicit ConditionSet(int64_t newLiteral, const ConditionsStore *conditions);

	// Load a set of conditions from the children of this node. Prints a
	// warning if the conditions cannot be parsed from the node.
	void Load(const DataNode &node, const ConditionsStore *conditions);

	/// Save toplevel set of conditions as a tree of children under a dataNode.
	void Save(DataWriter &out) const;

	/// Save a set of conditions as a tree of children under a dataNode.
	void SaveAsTree(DataWriter &out) const;

	/// Save the condition inline. Can be part of a bigger conditionSet.
	void SaveInline(DataWriter &out) const;

	// Change this condition to always be false.
	void MakeNever();

	// Check if there are any entries in this set.
	bool IsEmpty() const;

	// Check if this condition set contains valid data.
	bool IsValid() const;

	// Check if the player's conditions satisfy this set of expressions.
	bool Test() const;

	// Evaluate this expression into a numerical value. (The value can also be used as boolean.)
	int64_t Evaluate() const;

	/// Parse the remainder of a node into this ConditionSet.
	bool ParseNode(const DataNode &node, int &tokenNr);

	/// Optimize this node, this optimization also removes intermediate sections that were used for tracking brackets.
	bool Optimize(const DataNode &node);

	/// Get the names of the conditions that are queried by this ConditionSet.
	std::set<std::string> RelevantConditions() const;


private:
	/// Parse a node completely into this expression; all tokens on the line and all children if there are any.
	bool ParseFromStart(const DataNode &node);

	/// Parse the children under 'and'-nodes, 'or'-nodes, or the toplevel-node (which acts as and-node). The
	/// expression-operator should already have been set before calling this function.
	bool ParseChildren(const DataNode &node);

	/// Parse a minimal complete expression from the tokens into the (empty) expression.
	///
	/// @param node The node to report parse-errors on, if any occur.
	/// @param lineTokens Tokens to use (and pop from) for parsing.
	bool ParseMini(const DataNode &node, int &tokenNr);

	/// Parse as many tokens as possible.
	///
	/// @param node The node to report parse-errors on, if any occur.
	/// @param lineTokens Tokens to use (and pop from) for parsing.
	bool ParseGreedy(const DataNode &node, int &tokenNr);

	/// Parse a function (after a max or min keyword).
	///
	/// @param node The node to report parse-errors on, if any occur.
	/// @param lineTokens Tokens to use (and pop from) for parsing.
	bool ParseFunctionBody(const DataNode &node, int &tokenNr);

	/// Helper function to parse an infix operator and the subexpression after it.
	/// Should be called on ConditionSets that already have at least 1 sub-expression in them.
	///
	/// @param node The node to report parse-errors on, if any occur.
	/// @param lineTokens Tokens to use (and pop from) for parsing.
	bool ParseFromInfix(const DataNode &node, int &tokenNr, int parentPrecedence);

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
	/// - Clears the sub-expressions and sets the operator to INVALID.
	bool FailParse();

	/// Handles a failure in parsing;
	/// - Reports the failure using PrintTrace() in the given DataNode.
	/// - Clears the sub-expressions and sets the operator to INVALID.
	///
	/// @param node Node on which to report the failures (using node.PrintTrace()).
	/// @param failText The reason why parsing is failing. (Will be used as output for node.PrintTrace()).
	/// @return false So that is can be used as a one-liner for failures.
	bool FailParse(const DataNode &node, const std::string &failText);


private:
	// A pointer to the ConditionsStore that this set is evaluating.
	const ConditionsStore *conditions = nullptr;

	/// Sets of condition tests can contain nested sets of tests. Each set is
	/// combined using the expression operator that determines how the nested
	/// sets are to be combined.
	/// Using an `and`-operator with no sub-expressions as safe initial value.
	ExpressionOp expressionOperator = ExpressionOp::AND;
	/// Literal part of the expression, if this is a literal terminal.
	int64_t literal = 0;
	/// Condition variable that is used in this expression, if this is a condition variable.
	std::string conditionName;
	/// Nested sets of conditions to be tested.
	std::vector<ConditionSet> children;

	// Let the assignment class call internal functions and parsers.
	friend class ConditionAssignments;
};
