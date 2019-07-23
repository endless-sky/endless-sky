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
#include "Random.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

namespace {
	typedef int64_t (*BinFun)(int64_t, int64_t);
	BinFun Op(const string &op)
	{
		// This map defines functions that each "operator" should be mapped to.
		// In each function "a" is the condition's current value and "b" is the
		// integer value given as the other argument of the operator.
		// Test operators return 0 (false) or 1 (true).
		// "Apply" operators return the value that the condition should have
		// after applying the expression.
		static const map<string, BinFun> opMap = {
			{"==", [](int64_t a, int64_t b) -> int64_t { return a == b; }},
			{"!=", [](int64_t a, int64_t b) -> int64_t { return a != b; }},
			{"<", [](int64_t a, int64_t b) -> int64_t { return a < b; }},
			{">", [](int64_t a, int64_t b) -> int64_t { return a > b; }},
			{"<=", [](int64_t a, int64_t b) -> int64_t { return a <= b; }},
			{">=", [](int64_t a, int64_t b) -> int64_t { return a >= b; }},
			{"=", [](int64_t a, int64_t b) { return b; }},
			{"+=", [](int64_t a, int64_t b) { return a + b; }},
			{"-=", [](int64_t a, int64_t b) { return a - b; }},
			{"<?=", [](int64_t a, int64_t b) { return min(a, b); }},
			{">?=", [](int64_t a, int64_t b) { return max(a, b); }}
		};
		
		auto it = opMap.find(op);
		return (it != opMap.end() ? it->second : nullptr);
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
	if(node.Size() == 2)
	{
		if(!Add(node.Token(0), node.Token(1)))
			node.PrintTrace("Unrecognized condition expression:");
	}
	else if(node.Size() == 3)
	{
		if(node.IsNumber(2))
		{
			double value = node.Value(2);
			if(value > static_cast<double>(numeric_limits<int64_t>::max())
					|| value < static_cast<double>(numeric_limits<int64_t>::min()))
			{
				node.PrintTrace("Unrepresentable condition value " + to_string(value) + ":");
			}
			else if(!Add(node.Token(0), node.Token(1), static_cast<int64_t>(value)))
				node.PrintTrace("Unrecognized condition expression:");
		}
		else
		{
			if(!Add(node.Token(0), node.Token(1), node.Token(2)))
				node.PrintTrace("Unrecognized condition expression:");
		}
	}
	else if(node.Size() == 1 && node.Token(0) == "never")
		expressions.emplace_back("", "!=", 0);
	else if(node.Size() == 1 && (node.Token(0) == "and" || node.Token(0) == "or"))
	{
		// The "and" and "or" keywords introduce a nested condition set.
		children.emplace_back(node);
	}
	else
		node.PrintTrace("Unrecognized condition expression:");
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
	
	return true;
}



// Add a binary operator line to the list of expressions.
bool ConditionSet::Add(const string &name, const string &op, int64_t value)
{
	// If the operator is recognized, map it to a binary function.
	BinFun fun = Op(op);
	if(!fun)
		return false;
	
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
	
	expressions.emplace_back(name, op, 0);
	expressions.back().strValue = strValue;
	return true;
}



// Check if the given condition values satisfy this set of conditions.
bool ConditionSet::Test(const map<string, int64_t> &conditions) const
{
	for(const Expression &expression : expressions)
	{
		auto firstValue = TokenValue(0, expression.name, conditions);
		auto secondValue = TokenValue(expression.value, expression.strValue, conditions);
		bool result = expression.fun(firstValue, secondValue);
		// If this is a set of "and" conditions, bail out as soon as one of them
		// returns false. If it is an "or", bail out if anything returns true.
		if(result == isOr)
			return result;
	}
	for(const ConditionSet &child : children)
	{
		bool result = child.Test(conditions);
		if(result == isOr)
			return result;
	}
	// If this is an "and" condition, we got here because all the above conditions
	// returned true, so we should return true. If it is an "or," we got here because
	// no condition returned true, so we should return false.
	return !isOr;
}



// Modify the given set of conditions.
void ConditionSet::Apply(map<string, int64_t> &conditions) const
{
	for(const Expression &expression : expressions)
	{
		auto &c = conditions[expression.name];
		auto value = TokenValue(expression.value, expression.strValue, conditions);
		c = expression.fun(c, value);
	}
	// Note: "and" and "or" make no sense for "Apply()," so a condition set that
	// is meant to be applied rather than tested should never include them. But
	// just in case, apply anything included in a nested condition:
	for(const ConditionSet &child : children)
		child.Apply(conditions);
}



// Check if the passed token is numeric or a string which has to be replaced, and return its value
int64_t ConditionSet::TokenValue(int64_t numValue, const string &strValue, const map<string, int64_t> &conditions) const
{
	auto value = numValue;
	// Special case: if the string of the token is "random," that means to
	// generate a random number from 0 to 99 each time it is queried.
	if(strValue == "random")
		value = Random::Int(100);
	else
	{
		auto it = conditions.find(strValue);
		if(it != conditions.end())
			value = it->second;
	}
	return value;
}



// Constructor for an expression.
ConditionSet::Expression::Expression(const string &name, const string &op, int64_t value)
	: name(name), op(op), fun(Op(op)), value(value)
{
}
