/* ConditionSet.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONDITION_SET_H_
#define CONDITION_SET_H_

#include <map>
#include <string>
#include <vector>

class DataNode;
class DataWriter;



// A condition set is a collection of operations on the player's set of named
// "conditions". This includes "test" operations that just check the values of
// those conditions, and other operations that can be "applied" to change the
// values.
class ConditionSet {
public:
	using Conditions = std::map<std::string, int>;
	ConditionSet() = default;
	// Construct and Load() at the same time.
	ConditionSet(const DataNode &node);
	
	// Load a set of conditions from the children of this node. Prints a
	// warning if an and/or node contains assignment expressions.
	void Load(const DataNode &node);
	// Save a set of conditions.
	void Save(DataWriter &out) const;
	
	// Check if there are any entries in this set.
	bool IsEmpty() const;
	
	// Read a single condition from a data node.
	void Add(const DataNode &node);
	bool Add(const std::string &firstToken, const std::string &secondToken);
	bool Add(const std::string &name, const std::string &op, int value);
	bool Add(const std::string &name, const std::string &op, const std::string &strValue);
	
	// Check if the given condition values satisfy this set of expressions. First applies
	// all assignment expressions to create any temporary conditions, then evaluates.
	bool Test(const Conditions &conditions) const;
	// Modify the given set of conditions with this ConditionSet.
	// (Order of operations is like the order of specification: all sibling
	// expressions are applied, then any and/or nodes are applied.)
	void Apply(Conditions &conditions) const;
	
	
private:
	// Compare this set's expressions and the union of created and supplied conditions.
	bool TestSet(const Conditions &conditions, const Conditions &created) const;
	// Evaluate this set's assignment expressions and store the result in "created" (for use by TestSet).
	void TestApply(const Conditions &conditions, Conditions &created) const;
	
	
private:
	// This class represents a single expression involving a condition,
	// either testing what value it has, or modifying it in some way.
	class Expression {
	public:
		Expression(const std::string &name, const std::string &op, int value);
		
		// This is the name of the condition that this entry operates on.
		std::string name;
		// String representation of the Expression's binary function.
		std::string op;
		// Pointer to a binary function that defines the assignment or
		// comparison operation to be performed.
		int (*fun)(int, int);
		// Constant value specified in the expression.
		int value;
		// Allow for dynamic values.
		std::string strValue;
	};
	
	
private:
	// Sets of condition tests can contain nested sets of tests. Each set is
	// either an "and" grouping (meaning every condition must be true to satisfy
	// it) or an "or" grouping where only one condition needs to be true.
	bool isOr = false;
	// If this set contains assignment expressions. If true, the
	// Test() method must first apply them before testing any conditions.
	bool hasAssign = false;
	// Conditions that this set tests or applies.
	std::vector<Expression> expressions;
	// Nested sets of conditions to be tested.
	std::vector<ConditionSet> children;
};



#endif
