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
			{">?=", [](int a, int b) { return max(a, b); }},
			{"*", [](int a, int b) { return a * b; }},
			{"+", [](int a, int b) { return a + b; }},
			{"-", [](int a, int b) { return a - b; }},
			{"/", [](int a, int b) { return b ? a / b : numeric_limits<int>::max(); }}
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
	
	bool IsSimple(const string &op)
	{
		static const set<string> simple = {
			"*", "+", "-", "/", "(", ")"
		};
		return simple.count(op);
	}
	
	// Converts the given vector of strings into a vector of ints.
	vector<int> SubstituteValues(const vector<string> &side, const map<string, int> &conditions, const map<string, int> &created)
	{
		vector<int> result;
		for(const string &str : side)
		{
			if(DataNode::IsNumber(str))
				result.emplace_back(static_cast<int>(DataNode::Value(str)));
			else if(str == "random")
				result.emplace_back(Random::Int(100));
			else
			{
				const auto &temp = created.find(str);
				const auto &perm = conditions.find(str);
				if(temp != created.end())
					result.emplace_back(temp->second);
				else if(perm != conditions.end())
					result.emplace_back(perm->second);
				else
					result.emplace_back(0);
			}
		}
		return result;
	}
	
	bool UsedAll(vector<bool> &status)
	{
		for(const bool &v : status)
			if(!v)
				return false;
		return true;
	}
	
	// Get the operand's index within the data vector during evaluation.
	size_t LeftOperand(const vector<string> &tokens, const vector<int> &dataDest, const size_t &workingIndex)
	{
		// The left operand is at the first non-empty tokens index
		// if starting from the working index and moving backwards.
		size_t left = workingIndex;
		while(tokens.at(left).empty() && left > 0)
			--left;
		// Trace any used data to find the latest result.
		while(dataDest.at(left) > 0)
			left = dataDest.at(left);
		
		return left;
	}
	
	// Get the operand's index within the data vector during evaluation.
	size_t RightOperand(const vector<string> &tokens, const vector<int> &dataDest, const size_t &workingIndex)
	{
		// The right operand is at the first non-empty tokens index
		// if starting just past the working index and moving forward.
		size_t right = workingIndex + 1;
		while(tokens.at(right).empty() && right < tokens.size() - 2)
			++right;
		// Trace any used data to find the latest result.
		while(dataDest.at(right) > 0)
			right = dataDest.at(right);
		
		return right;
	}
	
	void PrintConditionError(const vector<string> &side)
	{
		string message = "Error decomposing complex condition expression:\nFound:\t";
		for(const string &str : side)
			message += " \"" + str + "\"";
		Files::LogError(message);
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
		expression.Save(out);
	
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
	
	hasAssign |= !expressions.back().IsTestable();
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
		if(!expression.IsTestable())
			expression.Apply(conditions, unused);
	
	for(const ConditionSet &child : children)
		child.Apply(conditions);
}



// Check if this set is satisfied by either the created, temporary conditions, or the given conditions.
bool ConditionSet::TestSet(const Conditions &conditions, const Conditions &created) const
{
	// Not all expressions may be testable: some may have been used to form the "created" condition map.
	for(const Expression &expression : expressions)
		if(expression.IsTestable())
		{
			bool result = expression.Test(conditions, created);
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
		if(!expression.IsTestable())
			expression.TestApply(conditions, created);
	
	for(const ConditionSet &child : children)
		child.TestApply(conditions, created);
}



// Constructor for an expression.
ConditionSet::Expression::Expression(const string &name, const string &op, int value, const vector<string> left, const vector<string> right)
	: name(name), op(op), fun(Op(op)), value(value), left(left), right(right)
{
}



void ConditionSet::Expression::Save(DataWriter &out) const
{
	out.Write(name, op, (!value && !strValue.empty()) ? strValue : to_string(value));
}



// Create a loggable string (for PrintTrace).
string ConditionSet::Expression::ToString() const
{
	return name + " \"" + op + "\" " + ((!value && !strValue.empty()) ? strValue : to_string(value));
}



string ConditionSet::Expression::Name() const
{
	return name;
}



bool ConditionSet::Expression::IsTestable() const
{
	return IsComparison(op);
}



bool ConditionSet::Expression::Test(const Conditions &conditions, const Conditions &created) const
{
	int firstValue = TokenValue(0, Name(), conditions, created);
	int secondValue = TokenValue(this->value, this->strValue, conditions, created);
	return fun(firstValue, secondValue);
}



// Assign the computed value to the desired condition.
void ConditionSet::Expression::Apply(Conditions &conditions, Conditions &created) const
{
	int &c = conditions[Name()];
	int value = TokenValue(this->value, this->strValue, conditions, created);
	c = fun(c, value);
}



// Assign the computed value to the desired temporary condition.
void ConditionSet::Expression::TestApply(const Conditions &conditions, Conditions &created) const
{
	int &c = created[Name()];
	int value = TokenValue(this->value, this->strValue, conditions, created);
	c = fun(c, value);
}



// Constructor for one side of a complex expression (supports multiple simple operators).
ConditionSet::Expression::SubExpression::SubExpression(const vector<string> &side)
{
	if(side.empty())
		return;
	
	ParseSide(side);
	// Create the sequence vector using operator precedence.
	GenerateSequence();
}



// Convert the tokens and operators back to a string, for use in the save file.
const string ConditionSet::Expression::SubExpression::ToString() const
{
	string out;
	static const string SPACE = " ";
	size_t i = 0;
	for( ; i < operators.size(); ++i)
	{
		out += tokens[i];
		out += SPACE;
		out += operators[i];
		out += SPACE;
	}
	// The tokens vector may contain more values than the operators vector.
	for( ; i < tokens.size(); ++i)
		out += tokens[i];
	return out;
}



// Evaluate the SubExpression using the given condition maps.
int ConditionSet::Expression::SubExpression::Evaluate(const Conditions &conditions, const Conditions &created) const
{
	// Sanity check.
	if(tokens.empty())
		return 0;
	
	// For SubExpressions with no Operations (i.e. simple conditions), tokens will consist
	// of only the condition or numeric value to be returned as-is after substitution.
	vector<int> data(SubstituteValues(tokens, conditions, created));
	
	if(!sequence.empty())
	{
		// Each Operation adds to the end of the data vector.
		data.reserve(operatorCount + data.size());
		for(const Operation &op : sequence)
			data.emplace_back(op.fun(data[op.a], data[op.b]));
	}
	
	return data.back();
}



// Parse the input vector into the tokens and operators vectors. Parentheses are
// considered simple operators that also cause an empty string insert to tokens.
void ConditionSet::Expression::SubExpression::ParseSide(const vector<string> &side)
{
	static const string EMPTY;
	int parentheses = 0;
	// Construct the tokens and operators vectors.
	for(size_t i = 0; i < side.size(); ++i)
	{
		if(side[i] == "(" || side[i] == ")")
		{
			// Ensure reconstruction by adding a blank token.
			tokens.emplace_back(EMPTY);
			operators.emplace_back(side[i]);
			++parentheses;
		}
		else if(IsSimple(side[i]))
		{
			// Normal operators do not need a token insertion.
			operators.emplace_back(side[i]);
			++operatorCount;
		}
		else
			tokens.emplace_back(side[i]);
	}
	
	if(tokens.empty() || !operatorCount)
		operators.clear();
	else if(parentheses % 2 != 0)
	{
		PrintConditionError(side);
		tokens.clear();
		operators.clear();
	}
	// Remove empty strings that wrap simple conditions.
	if(operators.empty() && !tokens.empty())
		for(auto it = tokens.begin(); it != tokens.end();)
		{
			if((*it).empty())
				it = tokens.erase(it);
			else
				++it;
		}
}



// Parse the token and operators vectors to make the sequence vector.
void ConditionSet::Expression::SubExpression::GenerateSequence()
{
	// Simple conditions have only a single token and no operators.
	if(tokens.empty() || operators.empty())
		return;
	static const map<string, int> precedence = {
		{"(", 0}, {")", 0},
		{"+", 1}, {"-", 1},
		{"*", 2}, {"/", 2}
	};
	// Use two boolean vectors to indicate when a token or operator has
	// been used already, and should be ignored when seeking.
	size_t dest = tokens.size();
	vector<bool> usedOps(operators.size(), false);
	// Read the operators vector just once by using a stack.
	vector<size_t> opStack;
	// Track where each subexpression places each operator's data. Each
	// operator generates a new data entry.
	vector<int> dataDest(dest + operatorCount, -1);
	int opIndex = 0;
	while(!UsedAll(usedOps))
	{
		while(true)
		{
			// Stack ops until one of lower or equal precedence is found, then evaluate the higher one first.
			if(opStack.empty() || operators.at(opIndex) == "("
					|| (precedence.at(operators.at(opIndex)) > precedence.at(operators.at(opStack.back()))))
			{
				opStack.push_back(opIndex);
				// Mark this operator as used and advance.
				usedOps.at(opIndex++) = true;
				break;
			}
			
			size_t workingIndex = opStack.back();
			opStack.pop_back();
			
			// A left parentheses results in a no-op step.
			if(operators.at(workingIndex) == "(")
			{
				if(operators.at(opIndex) != ")")
				{
					Files::LogError("Did not find matched parentheses:");
					PrintConditionError(operators);
					tokens.clear();
					operators.clear();
					sequence.clear();
					return;
				}
				// "Use" the parentheses and advance operators.
				usedOps.at(opIndex++) = true;
				break;
			}
			// Otherwise, obtain the operand indices. The operator
			// is never a parentheses. The working index can never
			// exceed the size of the tokens vector.
			else
			{
				size_t leftOp = LeftOperand(tokens, dataDest, workingIndex);
				size_t rightOp = RightOperand(tokens, dataDest, workingIndex);
				
				// Bail out if the pointed token is empty string.
				// Only in-bounds indices might be empty.
				if((leftOp < tokens.size() && tokens.at(leftOp).empty())
						|| (rightOp < tokens.size() && tokens.at(rightOp).empty()))
				{
					Files::LogError("Unable to obtain valid operand for function \"" + operators.at(workingIndex) + "\" with tokens");
					PrintConditionError(tokens);
					tokens.clear();
					operators.clear();
					sequence.clear();
					return;
				}
				
				// Record the use of each operand by writing
				// where its latest value can be found.
				dataDest.at(leftOp) = dest;
				dataDest.at(rightOp) = dest;
				// Create the Operation.
				sequence.emplace_back(operators.at(workingIndex), leftOp, rightOp);
				// Update the operands' destination.
				++dest;
			}
		}
	}
	// Handle remaining operators (which cannot be parentheses).
	while(!opStack.empty())
	{
		size_t workingIndex = opStack.back();
		opStack.pop_back();
		
		if(operators.at(workingIndex) == "(" || operators.at(workingIndex) == ")")
		{
			Files::LogError("Mismatched parentheses:" + ToString());
			tokens.clear();
			operators.clear();
			sequence.clear();
			return;
		}
		
		size_t leftOp = LeftOperand(tokens, dataDest, workingIndex);
		size_t rightOp = RightOperand(tokens, dataDest, workingIndex);
		
		// Bail out if the pointed token is empty string.
		// Only in-bounds indices might be empty.
		if((leftOp < tokens.size() && tokens.at(leftOp).empty())
				|| (rightOp < tokens.size() && tokens.at(rightOp).empty()))
		{
			Files::LogError("Unable to obtain valid operand for function \"" + operators.at(workingIndex) + "\" with tokens");
			PrintConditionError(tokens);
			tokens.clear();
			operators.clear();
			sequence.clear();
			return;
		}
		
		// Record the use of each operand by writing
		// where its latest value can be found.
		dataDest.at(leftOp) = dest;
		dataDest.at(rightOp) = dest;
		// Create the Operation.
		sequence.emplace_back(operators.at(workingIndex), leftOp, rightOp);
		// Update the operands' destination.
		++dest;
	}
	// All operators and tokens should now have been used.
	Files::LogError("Parsed:" + ToString() + "\nFound: " + to_string(operatorCount) + " true operators and made " + to_string(sequence.size()) + " operations.");
}



// Constructor for an Operation, indicating the binary function and the
// indices of its operands within the evaluation-time data vector.
ConditionSet::Expression::SubExpression::Operation::Operation(const string &op, size_t &a, size_t &b)
	: fun(Op(op)), a(a), b(b)
{
}
