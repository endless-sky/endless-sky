/* ConditionSet.cpp
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

#include "ConditionSet.h"

#include "ConditionsStore.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Logger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>
#include <utility>

using namespace std;

namespace
{
	typedef int64_t (*BinFun)(int64_t, int64_t);
	BinFun Op(ConditionSet::ExpressionOp op)
	{
		// This map defines functions that each "operator" should be mapped to.
		// In each function "a" is the condition's current value and "b" is the
		// integer value given as the other argument of the operator.
		// Test operators return 0 (false) or 1 (true).
		// "Apply" operators return the value that the condition should have
		// after applying the expression.
		static const map<ConditionSet::ExpressionOp, BinFun> opMap = {
			{ConditionSet::ExpressionOp::OP_EQ, [](int64_t a, int64_t b) -> int64_t { return a == b; }},
			{ConditionSet::ExpressionOp::OP_NE, [](int64_t a, int64_t b) -> int64_t { return a != b; }},
			{ConditionSet::ExpressionOp::OP_LT, [](int64_t a, int64_t b) -> int64_t { return a < b; }},
			{ConditionSet::ExpressionOp::OP_GT, [](int64_t a, int64_t b) -> int64_t { return a > b; }},
			{ConditionSet::ExpressionOp::OP_LE, [](int64_t a, int64_t b) -> int64_t { return a <= b; }},
			{ConditionSet::ExpressionOp::OP_GE, [](int64_t a, int64_t b) -> int64_t { return a >= b; }},
			{ConditionSet::ExpressionOp::OP_MOD, [](int64_t a, int64_t b) { return b ? a % b : a; }},
			{ConditionSet::ExpressionOp::OP_MUL, [](int64_t a, int64_t b) { return a * b; }},
			{ConditionSet::ExpressionOp::OP_ADD, [](int64_t a, int64_t b) { return a + b; }},
			{ConditionSet::ExpressionOp::OP_SUB, [](int64_t a, int64_t b) { return a - b; }},
			{ConditionSet::ExpressionOp::OP_DIV, [](int64_t a, int64_t b) { return b ? a / b : numeric_limits<int64_t>::max(); }}
		};

		auto it = opMap.find(op);
		return (it != opMap.end() ? it->second : nullptr);
	}

	/// Map string tokens to precedence and internal operators.
	const auto CS_TOKEN_CONVERSION = map<const string, ConditionSet::ExpressionOp>{
		// Infix arithmetic multiply, divide and modulo have a higher precedence than add and subtract.
		{ "*", ConditionSet::ExpressionOp::OP_MUL },
		{ "/", ConditionSet::ExpressionOp::OP_DIV },
		{ "%", ConditionSet::ExpressionOp::OP_MOD },
		// Infix arithmetic operators add and subtract have the same precedence.
		{ "+", ConditionSet::ExpressionOp::OP_ADD },
		{ "-", ConditionSet::ExpressionOp::OP_SUB },
		// Infix boolean equality operators have a lower precedence than their arithmetic counterparts.
		{ "==", ConditionSet::ExpressionOp::OP_EQ },
		{ "!=", ConditionSet::ExpressionOp::OP_NE },
		{ ">", ConditionSet::ExpressionOp::OP_GT },
		{ "<", ConditionSet::ExpressionOp::OP_LT },
		{ ">=", ConditionSet::ExpressionOp::OP_GE },
		{ "<=", ConditionSet::ExpressionOp::OP_LE },
		// Parent-type operators have a low precedence in Endless-Sky, because they are on outer parent/child sections.
		{ "and", ConditionSet::ExpressionOp::OP_AND },
		{ "or", ConditionSet::ExpressionOp::OP_OR },
	};


	/// Get the precedence of an operator.
	int Precedence(const ConditionSet::ExpressionOp op)
	{
		switch(op)
		{
			case ConditionSet::ExpressionOp::OP_INVALID:
				return 9;
			case ConditionSet::ExpressionOp::OP_LIT:
			case ConditionSet::ExpressionOp::OP_VAR:
				return 8;
			case ConditionSet::ExpressionOp::OP_MUL:
			case ConditionSet::ExpressionOp::OP_DIV:
			case ConditionSet::ExpressionOp::OP_MOD:
				return 6;
			case ConditionSet::ExpressionOp::OP_ADD:
			case ConditionSet::ExpressionOp::OP_SUB:
				return 5;
			case ConditionSet::ExpressionOp::OP_EQ:
			case ConditionSet::ExpressionOp::OP_NE:
			case ConditionSet::ExpressionOp::OP_GT:
			case ConditionSet::ExpressionOp::OP_LT:
			case ConditionSet::ExpressionOp::OP_GE:
			case ConditionSet::ExpressionOp::OP_LE:
				return 3;
			default:
				// Precedence for OP_AND, OP_OR
				return 0;
		}
	}


	ConditionSet::ExpressionOp ParseOperator(const string &stringToken)
	{
		auto it = CS_TOKEN_CONVERSION.find(stringToken);
		if(it != CS_TOKEN_CONVERSION.end())
			return it->second;

		// If nothing matches, then we get the default OP_INVALID value.
		return ConditionSet::ExpressionOp::OP_INVALID;
	}
}



// Construct and Load() at the same time.
ConditionSet::ConditionSet(const DataNode &node)
{
	Load(node);
}



// Construct a terminal with a literal value;
ConditionSet::ConditionSet(int64_t newLiteral)
{
	expressionOperator = ExpressionOp::OP_LIT;
	literal = newLiteral;
}



// Load a set of conditions from the children of this node.
void ConditionSet::Load(const DataNode &node)
{
	// The top-node is always an 'and' node, without the keyword.
	expressionOperator = ExpressionOp::OP_AND;
	ParseBooleanChildren(node);
}



// Save a set of conditions.
void ConditionSet::Save(DataWriter &out) const
{
	// Default should be AND, so if it is, then just write the subsets.
	// If this condition got optimized beyond OP_AND, then re-add the OP_AND by writing the current condition in full.
	if(expressionOperator == ExpressionOp::OP_AND)
		for(const auto &child : children)
		{
			child.SaveSubset(out);
			out.Write();
		}
	else
		SaveSubset(out);
}



void ConditionSet::SaveChild(int childNr, DataWriter &out) const
{
	const ConditionSet &child = children[childNr];
	bool needBrackets = child.children.size() > 0;

	if(needBrackets)
		out.WriteToken("(");
	children[childNr].SaveSubset(out);
	if(needBrackets)
		out.WriteToken(")");
}



// Save a subset of conditions, by writing out tokens (without a newline).
void ConditionSet::SaveSubset(DataWriter &out) const
{
	string opTxt = "";
	auto it = find_if(CS_TOKEN_CONVERSION.begin(), CS_TOKEN_CONVERSION.end(),
		[this](const std::pair<const string, ConditionSet::ExpressionOp> &e) {
			return e.second == expressionOperator;
		});
	if(it != CS_TOKEN_CONVERSION.end())
		opTxt = it->first;

	switch(expressionOperator)
	{
	case ExpressionOp::OP_INVALID:
		out.WriteToken("never");
		break;
	case ExpressionOp::OP_VAR:
		out.WriteToken(conditionName);
		break;
	case ExpressionOp::OP_LIT:
		out.WriteToken(literal);
		break;
	case ExpressionOp::OP_ADD:
	case ExpressionOp::OP_SUB:
	case ExpressionOp::OP_MUL:
	case ExpressionOp::OP_DIV:
	case ExpressionOp::OP_MOD:
	case ExpressionOp::OP_EQ:
	case ExpressionOp::OP_NE:
	case ExpressionOp::OP_LE:
	case ExpressionOp::OP_GE:
	case ExpressionOp::OP_LT:
	case ExpressionOp::OP_GT:
		if(children.empty())
		{
			out.WriteToken("never");
			break;
		}
		SaveChild(0, out);
		for(unsigned int i = 1; i < children.size(); ++i)
		{
			out.WriteToken(opTxt);
			SaveChild(i, out);
		}
		break;
	case ExpressionOp::OP_AND:
	case ExpressionOp::OP_OR:
		out.Write(opTxt);
		out.BeginChild();
		for(const auto &child : children)
		{
			child.SaveSubset(out);
			out.Write();
		}
		out.EndChild();
		break;
	case ExpressionOp::OP_NOT:
	case ExpressionOp::OP_HAS:
		if(children.empty())
		{
			out.WriteToken("never");
			break;
		}
		out.WriteToken(opTxt);
		SaveChild(0, out);
		break;
	default:
		out.WriteToken("never");
		break;
	};
}



void ConditionSet::MakeNever()
{
	children.clear();
	expressionOperator = ExpressionOp::OP_LIT;
	literal = 0;
}



// Check if there are any entries in this set.
// Invalid ConditionSets are also considered empty.
bool ConditionSet::IsEmpty() const
{
	return
		(expressionOperator == ExpressionOp::OP_AND && children.size() == 0) ||
		(expressionOperator == ExpressionOp::OP_INVALID);
}



// Check if the conditionset contains valid data
bool ConditionSet::IsValid() const
{
	return expressionOperator != ExpressionOp::OP_INVALID;
}



// Check if the given condition values satisfy this set of conditions.
bool ConditionSet::Test(const ConditionsStore &conditions) const
{
	return Evaluate(conditions);
}



int64_t ConditionSet::Evaluate(const ConditionsStore &conditionsStore) const
{
	switch(expressionOperator)
	{
		case ExpressionOp::OP_VAR:
			return conditionsStore.Get(conditionName);
		case ExpressionOp::OP_LIT:
			return literal;
		case ExpressionOp::OP_AND:
		{
			// An empty AND section returns true.
			if(children.empty())
				return 1;

			int64_t result = 0;
			for(const ConditionSet &child : children)
			{
				int64_t childResult = child.Evaluate(conditionsStore);
				if(!childResult)
					return 0;
				// Assign the first non-zero result to the result variable.
				if(!result)
					result = childResult;
			}
			return result;
		}
		case ExpressionOp::OP_OR:
			for(const ConditionSet &child : children)
			{
				int64_t childResult = child.Evaluate(conditionsStore);
				// Return the first non-zero result.
				if(childResult)
					return childResult;
			}
			return 0;
		default:
			break;
	}

	// If we have an accumulator function and children, then let's use the accumulator on the children.
	BinFun accumulatorOp = Op(expressionOperator);
	if(accumulatorOp != nullptr && !children.empty())
		return accumulate(next(children.begin()), children.end(), children[0].Evaluate(conditionsStore),
			[&accumulatorOp, &conditionsStore](int64_t accumulated, ConditionSet b) -> int64_t {
				return accumulatorOp(accumulated, b.Evaluate(conditionsStore));
		});

	// If we don't have an accumulator function, or no children, then return the default value.
	return 0;
}



// Get the names of the conditions that are relevant for this ConditionSet.
set<string> ConditionSet::RelevantConditions() const
{
	set<string> result;
	// Add the name from this set, if it is a VAR type operator.
	if(expressionOperator == ExpressionOp::OP_VAR)
		result.emplace(conditionName);
	// Add the names from the children.
	for(const auto &child : children)
		for(const auto &rc : child.RelevantConditions())
			result.emplace(rc);
	return result;
}


bool ConditionSet::ParseNode(const DataNode &node)
{
	// Special handling for 'and' and 'or' nodes.
	if(node.Size() == 1)
	{
		if(node.Token(0) == "and")
		{
			expressionOperator = ExpressionOp::OP_AND;
			return ParseBooleanChildren(node);
		}
		if(node.Token(0) == "or")
		{
			expressionOperator = ExpressionOp::OP_OR;
			return ParseBooleanChildren(node);
		}
	}

	// Nodes beyond this point should not have children.
	if(node.HasChildren())
		return FailParse(node, "unexpected child-nodes under toplevel");

	// Special handling for 'never', 'has' and 'not' nodes.
	if(node.Token(0) == "never")
	{
		if(node.Size() > 1)
			return FailParse(node, "tokens found after never keyword");

		expressionOperator = ExpressionOp::OP_LIT;
		literal = 0;
		return true;
	}
	if(node.Token(0) == "has")
	{
		if(node.Size() != 2 || !DataNode::IsConditionName(node.Token(1)))
			return FailParse(node, "has keyword requires a single condition");

		// Convert has keyword directly to the variable.
		expressionOperator = ExpressionOp::OP_VAR;
		conditionName = node.Token(1);
		return true;
	}
	if(node.Token(0) == "not")
	{
		if(node.Size() != 2 || !DataNode::IsConditionName(node.Token(1)))
			return FailParse(node, "not keyword requires a single condition");

		// Create `conditionName == 0` expression.
		expressionOperator = ExpressionOp::OP_EQ;
		children.emplace_back();
		children.back().expressionOperator = ExpressionOp::OP_VAR;
		children.back().conditionName = node.Token(1);
		children.emplace_back(0);
		return true;
	}

	int tokenNr = 0;
	if(!ParseNode(node, tokenNr))
		return false;

	return Optimize(node);
}



bool ConditionSet::ParseNode(const DataNode &node, int &tokenNr)
{
	// Nodes beyond this point should not have children.
	if(node.HasChildren())
		return FailParse(node, "unexpected child-nodes under arithmetic expression");

	// Parse initial expression.
	if(!ParseMini(node, tokenNr))
		return FailParse();

	// Check if we are done with just one expression.
	if(tokenNr >= node.Size())
		return true;

	// If there are more tokens, then we need to have an infix operator here.
	if(!ParseFromInfix(node, tokenNr, ExpressionOp::OP_AND))
		return FailParse();

	// Parsing from infix should have consumed and parsed all tokens.
	if(tokenNr < node.Size())
		return FailParse(node, "tokens found after parsing full expression");

	return true;
}



/// Optimize this node, this optimization also removes intermediate sections that were used for tracking brackets.
bool ConditionSet::Optimize(const DataNode &node)
{
	bool returnValue = true;
	// First optimize all the child nodes below.
	for(ConditionSet &child : children)
		returnValue &= child.Optimize(node);

	switch(expressionOperator)
	{
		case ExpressionOp::OP_AND:
		case ExpressionOp::OP_OR:
			// If we only have a single element, then replace the current OP/AND by its child.
			if(children.size() == 1)
				PromoteFirstChild(node);

			break;

		case ExpressionOp::OP_EQ:
		case ExpressionOp::OP_NE:
		case ExpressionOp::OP_LE:
		case ExpressionOp::OP_GE:
		case ExpressionOp::OP_LT:
		case ExpressionOp::OP_GT:
			// TODO: Optimize boolean equality operators.
			break;

		case ExpressionOp::OP_ADD:
		case ExpressionOp::OP_SUB:
		case ExpressionOp::OP_MUL:
		case ExpressionOp::OP_DIV:
		case ExpressionOp::OP_MOD:
			// TODO: Optimize arithmetic operators.
			break;

		case ExpressionOp::OP_HAS:
			// Optimize away HAS, we can directly use the expression below it.
			if(children.size() == 1)
				PromoteFirstChild(node);

			break;

		case ExpressionOp::OP_NOT:
		case ExpressionOp::OP_LIT:
		case ExpressionOp::OP_VAR:
		case ExpressionOp::OP_INVALID:
			break;
	}
	return returnValue;
}



bool ConditionSet::ParseBooleanChildren(const DataNode &node)
{
	if(!node.HasChildren())
		return FailParse(node, "child-nodes expected, found none");

	// Load all child nodes.
	for(const DataNode &child : node)
	{
		children.emplace_back();
		children.back().ParseNode(child);

		if(children.back().expressionOperator == ExpressionOp::OP_INVALID)
			return FailParse();
	}

	return true;
}



bool ConditionSet::ParseMini(const DataNode &node, int &tokenNr)
{
	if(tokenNr >= node.Size())
		return FailParse(node, "expected terminal or sub-expression, found none");

	// Any (sub)expression should start with one of the following:
	// - an opening bracket.
	// - a literal number terminal.
	// - a condition name terminal.
	// - has keyword (but this is already handled at a higher level)
	// - not keyword (but this is already handled at a higher level)

	// Handle any first open bracket, if we had any.
	bool hadOpenBracket = false;
	if(node.Token(tokenNr) == "(")
	{
		hadOpenBracket = true;
		++tokenNr;
		if(tokenNr >= node.Size())
			return FailParse(node, "missing sub-expression and closing bracket");
	}

	if(node.IsNumber(tokenNr))
	{
		expressionOperator = ExpressionOp::OP_LIT;
		literal = node.Value(tokenNr);
		++tokenNr;
	}
	else if(DataNode::IsConditionName(node.Token(tokenNr)))
	{
		expressionOperator = ExpressionOp::OP_VAR;
		conditionName = node.Token(tokenNr);
		++tokenNr;
	}
	else if(node.Token(tokenNr) == "(")
	{
		// We must already have handled an open-bracket to get here; this one goes into a sub-expression.
		children.emplace_back();
		children.back().ParseMini(node, tokenNr);
	}
	else
		return FailParse(node, "expected terminal or open-bracket");

	// Keep parsing until we get to the closing bracket, if we had an open bracket.
	while(hadOpenBracket)
	{
		if(tokenNr >= node.Size())
			return FailParse(node, "missing closing bracket");
		else if(node.Token(tokenNr) == ")")
		{
			// Remove the closing bracket.
			++tokenNr;
			hadOpenBracket = false;
			// Make sure that this bracketed section gets used as a single terminal.
			if(!PushDownFull(node))
				return FailParse();
		}
		else
			// If there are more tokens, then we need to have an infix operator here.
			// Use the precedence of the AND operator, since we want to parse to the closing bracket.
			if(!ParseFromInfix(node, tokenNr, ExpressionOp::OP_AND))
				return FailParse();
	}
	return true;
}



bool ConditionSet::ParseFromInfix(const DataNode &node, int &tokenNr, ExpressionOp parentOp)
{
	// Keep on parsing until we reach an end-state (error, end-of-tokens, closing-bracket, lower precedence token)
	while(true)
	{
		// At this point, we can expect one of the following:
		// - an infix-operator
		// - a closing bracket (hopefully matching an earlier open bracket)
		// - end of the tokens.

		// Reaching the end is fine, since we should have parsed a full terminal before this one.
		// Reaching a closing bracket also means we are done (the parent should handle it).
		if(tokenNr >= node.Size() || node.Token(tokenNr) == ")")
			return true;

		// Consume token and process it.
		ExpressionOp infixOp = ParseOperator(node.Token(tokenNr));
		switch(infixOp)
		{
			case ExpressionOp::OP_ADD:
			case ExpressionOp::OP_SUB:
			case ExpressionOp::OP_MUL:
			case ExpressionOp::OP_DIV:
			case ExpressionOp::OP_MOD:
			case ExpressionOp::OP_EQ:
			case ExpressionOp::OP_NE:
			case ExpressionOp::OP_LE:
			case ExpressionOp::OP_GE:
			case ExpressionOp::OP_LT:
			case ExpressionOp::OP_GT:
			{
				if(tokenNr + 1 >= node.Size())
					return FailParse(node, "expected terminal after infix operator \"" + node.Token(tokenNr) + "\"");

				// If the precedence of the new operator is less or equal than the parents operator, then let the parent handle it.
				if(Precedence(infixOp) <= Precedence(parentOp))
					return true;

				// If the precedence of the new operator is higher than the current operator, then parse the next
				// terminal into a new sub-expression.
				if((children.size() > 1) && (Precedence(expressionOperator) < Precedence(infixOp)))
				{
					if(!PushDownLast(node))
						return FailParse();
					if(!children.back().ParseFromInfix(node, tokenNr, expressionOperator))
						return FailParse();
					// The parser for the sub-expression handled everything with higher precedence. Start the loop over
					// to check what this parser needs to do next.
					continue;
				}

				// If the expression currently contains a terminal, then push it down.
				// Also push down the current expression if it has a higher or equal precedence to the new operator.
				if((children.size() == 0) || (children.size() > 1 && infixOp != expressionOperator &&
						Precedence(expressionOperator) >= Precedence(infixOp)))
					if(!PushDownFull(node))
						return FailParse();

				// If this expression contains only a single sub-expression, then we can apply the operator directly.
				if(children.size() == 1)
					expressionOperator = infixOp;

				// If we get the same operator as that we had, then let's just process it and continue the loop.
				if(infixOp == expressionOperator)
				{
					++tokenNr;
					if(!((children.emplace_back()).ParseMini(node, tokenNr)))
						return FailParse();

					continue;
				}
				return FailParse(node, "precedence confusion on infix operator");
			}
			default:
				return FailParse(node, "expected infix operator instead of \"" + node.Token(tokenNr) + "\"");
		}
	}
}



bool ConditionSet::PromoteFirstChild(const DataNode &node)
{
	// Need to copy first, before moving the vector.
	ConditionSet cs = children[0];
	expressionOperator = cs.expressionOperator;
	literal = cs.literal;
	conditionName = cs.conditionName;
	children = cs.children;

	return true;
}



bool ConditionSet::PushDownFull(const DataNode &node)
{
	ConditionSet ce(*this);
	children.clear();
	children.emplace_back(ce);
	expressionOperator = ExpressionOp::OP_AND;

	return true;
}



bool ConditionSet::PushDownLast(const DataNode &node)
{
	// Can only perform push-down if there is at least one expression to push down.
	if(children.empty())
		return FailParse(node, "cannot create sub-expression from void");

	ConditionSet ce = children.back();
	children.pop_back();
	children.emplace_back();
	children.back().children.push_back(std::move(ce));
	return true;
}



bool ConditionSet::FailParse()
{
	expressionOperator = ExpressionOp::OP_INVALID;
	children.clear();
	return false;
}



bool ConditionSet::FailParse(const DataNode &node, const string &failText)
{
	node.PrintTrace("Error: " + failText + ":");
	return FailParse();
}
