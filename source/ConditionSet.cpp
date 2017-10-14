/* ConditionSet.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ConditionSet.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "Random.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

using namespace std;

namespace {
	typedef int (*BinFun)(int, int);
	BinFun Op(const string &op)
	{
		// This map defines functions that each "operator" should be mapped to.
		// In each function "a" is the condition's current value and "b" is the
		// integer value given as the other argument of the operator.
		// Test operators return 0 (false) or 1 (true).
		// "Apply" operators return the value that the condition should have
		// after applying the expression.
		static const map<string, BinFun> opMap = {
			{"==", [](int a, int b) -> int { return a == b; }},
			{"!=", [](int a, int b) -> int { return a != b; }},
			{"<", [](int a, int b) -> int { return a < b; }},
			{">", [](int a, int b) -> int { return a > b; }},
			{"<=", [](int a, int b) -> int { return a <= b; }},
			{">=", [](int a, int b) -> int { return a >= b; }},
			{"=", [](int a, int b) { return b; }},
			{"*=", [](int a, int b) { return a * b; }},
			{"+=", [](int a, int b) { return a + b; }},
			{"-=", [](int a, int b) { return a - b; }},
			{"/=", [](int a, int b) { return b ? a / b : numeric_limits<int>::max(); }},
			{"<?=", [](int a, int b) { return min(a, b); }},
			{">?=", [](int a, int b) { return max(a, b); }}
		};
		
		auto it = opMap.find(op);
		return (it != opMap.end() ? it->second : nullptr);
	}
	
	// Indicate if the operation is a comparison or modifies the condition.
	bool IsComparison(const string &op)
	{
		static const set<string> comparison = {
			"==", "!=", "<", ">", "<=", ">="
		};
		return comparison.count(op);
	}
	
	// Check if the passed token is numeric or a string which has to be replaced, and return the
	// evaluated value. If the string value is a "created" condition (from TestApply()), use that,
	// otherwise find the value in the player's conditions.
	double TokenValue(int numValue, const string &strValue, const map<string, int> &conditions, const map<string, int> &created)
	{
		int value = numValue;
		// Special case: if the string of the token is "random," that means to
		// generate a random number from 0 to 99 each time it is queried.
		if(strValue == "random")
			value = Random::Int(100);
		else
		{
			// Prefer temporary conditions, since they may have the same
			// name as a condition stored in the player's list.
			auto temp = created.find(strValue);
			if(temp != created.end())
				return temp->second;
			
			auto perm = conditions.find(strValue);
			if(perm != conditions.end())
				return perm->second;
		}
		return value;
	}
}



// Construct and Load() at the same time.
ConditionSet::ConditionSet(const DataNode &node)
{
	Load(node);
}



// Load a set of conditions from the children of this node.
void ConditionSet::Load(const DataNode &node)
{
	isOr = (node.Token(0) == "or");
	for(const DataNode &child : node)
		Add(child);
}



// Save a set of conditions.
void ConditionSet::Save(DataWriter &out) const
{
	for(const Expression &expression : expressions)
	{
		if(!expression.value && !expression.strValue.empty())
			out.Write(expression.name, expression.op, expression.strValue);
		else
			out.Write(expression.name, expression.op, expression.value);
	}
	for(const ConditionSet &child : children)
	{
		out.Write(child.isOr ? "or" : "and");
		out.BeginChild();
		{
			child.Save(out);
		}
		out.EndChild();
	}
}



// Check if there are any entries in this set.
bool ConditionSet::IsEmpty() const
{
	return expressions.empty() && children.empty();
}



// Read a single condition from a data node.
void ConditionSet::Add(const DataNode &node)
{
	// Branch based on whether this line has two tokens (a unary operator) or
	// three tokens (a binary operator).
	static const string UNRECOGNIZED = "Unrecognized condition expression:";
	if(node.Size() == 2)
	{
		if(!Add(node.Token(0), node.Token(1)))
			node.PrintTrace(UNRECOGNIZED);
	}
	else if(node.Size() == 3)
	{
		if(node.IsNumber(2))
		{
			if(!Add(node.Token(0), node.Token(1), node.Value(2)))
				node.PrintTrace(UNRECOGNIZED);
		}
		else
		{
			if(!Add(node.Token(0), node.Token(1), node.Token(2)))
				node.PrintTrace(UNRECOGNIZED);
		}
	}
	else if(node.Size() == 1 && node.Token(0) == "never")
		expressions.emplace_back("", "!=", 0);
	else if(node.Size() == 1 && (node.Token(0) == "and" || node.Token(0) == "or"))
	{
		// The "and" and "or" keywords introduce a nested condition set.
		children.emplace_back(node);
		// If a child node has assignment operators, warn on load since
		// these will be processed after all non-child expressions.
		if(children.back().hasAssign)
			node.PrintTrace("Assignment expressions contained within and/or groups are applied last. This may be unexpected.");
	}
	else
		node.PrintTrace(UNRECOGNIZED);
}



// Add a unary operator line to the list of expressions.
bool ConditionSet::Add(const string &firstToken, const string &secondToken)
{
	// Each "unary" operator can be mapped to an equivalent binary expression.
	if(firstToken == "not")
		expressions.emplace_back(secondToken, "==", 0);
	else if(firstToken == "has")
		expressions.emplace_back(secondToken, "!=", 0);
	else if(firstToken == "set")
		expressions.emplace_back(secondToken, "=", 1);
	else if(firstToken == "clear")
		expressions.emplace_back(secondToken, "=", 0);
	else if(secondToken == "++")
		expressions.emplace_back(firstToken, "+=", 1);
	else if(secondToken == "--")
		expressions.emplace_back(firstToken, "-=", 1);
	else
		return false;
	
	hasAssign |= !IsComparison(expressions.back().op);
	return true;
}



// Add a binary operator line to the list of expressions.
bool ConditionSet::Add(const string &name, const string &op, int value)
{
	// If the operator is recognized, map it to a binary function.
	BinFun fun = Op(op);
	if(!fun)
		return false;
	
	hasAssign |= !IsComparison(op);
	expressions.emplace_back(name, op, value);
	return true;
}



// Add a binary operator line to the list of expressions with a string as value
bool ConditionSet::Add(const string &name, const string &op, const string &strValue)
{
	// If the operator is recognized, map it to a binary function.
	BinFun fun = Op(op);
	if(!fun)
		return false;
	
	hasAssign |= !IsComparison(op);
	expressions.emplace_back(name, op, 0);
	expressions.back().strValue = strValue;
	return true;
}



// Check if the given condition values satify this set of conditions. Performs any assignments
// on a temporary condition map, if this set mixes comparisons and modifications.
bool ConditionSet::Test(const Conditions &conditions) const
{
	// If this ConditionSet contains any expressions with operators that
	// modify the condition map, then they must be applied before testing,
	// to generate any temporary conditions needed.
	Conditions created;
	if(hasAssign)
		TestApply(conditions, created);
	return TestSet(conditions, created);
}



// Modify the given set of conditions.
void ConditionSet::Apply(Conditions &conditions) const
{
	Conditions unused;
	for(const Expression &expression : expressions)
		if(!IsComparison(expression.op))
		{
			int &c = conditions[expression.name];
			int value = TokenValue(expression.value, expression.strValue, conditions, unused);
			c = expression.fun(c, value);
		}
	
	for(const ConditionSet &child : children)
		child.Apply(conditions);
}



// Check if this set is satisfied by either the created, temporary conditions, or the given conditions.
bool ConditionSet::TestSet(const Conditions &conditions, const Conditions &created) const
{
	// Not all expressions may be testable: some may have been used to form the "created" condition map.
	for(const Expression &expression : expressions)
		if(IsComparison(expression.op))
		{
			int firstValue = TokenValue(0, expression.name, conditions, created);
			int secondValue = TokenValue(expression.value, expression.strValue, conditions, created);
			bool result = expression.fun(firstValue, secondValue);
			// If this is a set of "and" conditions, bail out as soon as one of them
			// returns false. If it is an "or", bail out if anything returns true.
			if(result == isOr)
				return result;
		}
	
	for(const ConditionSet &child : children)
	{
		bool result = child.TestSet(conditions, created);
		if(result == isOr)
			return result;
	}
	// If this is an "and" condition, all the above conditions were true, so return
	// true. If it is an "or," no condition returned true, so return false.
	return !isOr;
}



// Construct new, temporary conditions based on the assignment expressions in
// this ConditionSet and the values in the player's conditions map.
void ConditionSet::TestApply(const Conditions &conditions, Conditions &created) const
{
	for(const Expression &expression : expressions)
		if(!IsComparison(expression.op))
		{
			int &c = created[expression.name];
			int value = TokenValue(expression.value, expression.strValue, conditions, created);
			c = expression.fun(c, value);
		}
	
	for(const ConditionSet &child : children)
		child.TestApply(conditions, created);
}



// Constructor for a simple expression.
ConditionSet::Expression::Expression(const string &name, const string &op, int value)
	: name(name), op(op), fun(Op(op)), value(value)
{
}
