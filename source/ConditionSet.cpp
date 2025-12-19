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
			{ConditionSet::ExpressionOp::EQ, [](int64_t a, int64_t b) -> int64_t { return a == b; }},
			{ConditionSet::ExpressionOp::NE, [](int64_t a, int64_t b) -> int64_t { return a != b; }},
			{ConditionSet::ExpressionOp::LT, [](int64_t a, int64_t b) -> int64_t { return a < b; }},
			{ConditionSet::ExpressionOp::GT, [](int64_t a, int64_t b) -> int64_t { return a > b; }},
			{ConditionSet::ExpressionOp::LE, [](int64_t a, int64_t b) -> int64_t { return a <= b; }},
			{ConditionSet::ExpressionOp::GE, [](int64_t a, int64_t b) -> int64_t { return a >= b; }},
			{ConditionSet::ExpressionOp::MOD, [](int64_t a, int64_t b) { return b ? a % b : a; }},
			{ConditionSet::ExpressionOp::MUL, [](int64_t a, int64_t b) { return a * b; }},
			{ConditionSet::ExpressionOp::ADD, [](int64_t a, int64_t b) { return a + b; }},
			{ConditionSet::ExpressionOp::SUB, [](int64_t a, int64_t b) { return a - b; }},
			{ConditionSet::ExpressionOp::DIV, [](int64_t a, int64_t b) { return b ? a / b : numeric_limits<int64_t>::max(); }},
			{ConditionSet::ExpressionOp::MAX, [](int64_t a, int64_t b) { return max(a, b); }},
			{ConditionSet::ExpressionOp::MIN, [](int64_t a, int64_t b) { return min(a, b); }},
		};

		auto it = opMap.find(op);
		return (it != opMap.end() ? it->second : nullptr);
	}

	// Expressions that define how operators should be parsed and written
	constexpr uint64_t ONE = 1;
	constexpr uint64_t SINGLE_PARENT = ONE << 0; ///< Can be a single keyword in a node, with child-nodes.
	constexpr uint64_t INFIX_OPERATOR = ONE << 1; ///< Operator parsed and written as infix inbetween terminals.
	constexpr uint64_t FUNCTION_OPERATOR = ONE << 2; ///< Operator parsed and written as a function.
	constexpr uint64_t TERMINAL = ONE << 3; ///< Operator indicating a terminal node (number or condition variable).

	/// Map string tokens internal operators.
	const auto CS_TOKEN_CONVERSION = map<const string, pair<ConditionSet::ExpressionOp, uint64_t>>{
		// Infix arithmetic multiply, divide and modulo have a higher precedence than add and subtract.
		{ "*", { ConditionSet::ExpressionOp::MUL, INFIX_OPERATOR }},
		{ "/", { ConditionSet::ExpressionOp::DIV, INFIX_OPERATOR }},
		{ "%", { ConditionSet::ExpressionOp::MOD, INFIX_OPERATOR }},
		// Infix arithmetic operators add and subtract have the same precedence.
		{ "+", { ConditionSet::ExpressionOp::ADD, INFIX_OPERATOR }},
		{ "-", { ConditionSet::ExpressionOp::SUB, INFIX_OPERATOR }},
		// Infix boolean equality operators have a lower precedence than their arithmetic counterparts.
		{ "==", { ConditionSet::ExpressionOp::EQ, INFIX_OPERATOR }},
		{ "!=", { ConditionSet::ExpressionOp::NE, INFIX_OPERATOR }},
		{ ">", { ConditionSet::ExpressionOp::GT, INFIX_OPERATOR }},
		{ "<", { ConditionSet::ExpressionOp::LT, INFIX_OPERATOR }},
		{ ">=", { ConditionSet::ExpressionOp::GE, INFIX_OPERATOR }},
		{ "<=", { ConditionSet::ExpressionOp::LE, INFIX_OPERATOR }},
		// Parent-type operators have a low precedence in Endless-Sky, because they are on outer parent/child sections.
		// and, or, max and min can exist as infix-operators and as single parents.
		{ "and", { ConditionSet::ExpressionOp::AND, INFIX_OPERATOR | SINGLE_PARENT }},
		{ "or", { ConditionSet::ExpressionOp::OR, INFIX_OPERATOR | SINGLE_PARENT }},
		{ "max", { ConditionSet::ExpressionOp::MAX, FUNCTION_OPERATOR | SINGLE_PARENT }},
		{ "min", { ConditionSet::ExpressionOp::MIN, FUNCTION_OPERATOR | SINGLE_PARENT}},
	};


	/// Get the precedence of an operator.
	int Precedence(const ConditionSet::ExpressionOp op)
	{
		switch(op)
		{
			case ConditionSet::ExpressionOp::INVALID:
				return 9;
			case ConditionSet::ExpressionOp::LIT:
			case ConditionSet::ExpressionOp::VAR:
				return 8;
			case ConditionSet::ExpressionOp::MUL:
			case ConditionSet::ExpressionOp::DIV:
			case ConditionSet::ExpressionOp::MOD:
				return 6;
			case ConditionSet::ExpressionOp::ADD:
			case ConditionSet::ExpressionOp::SUB:
				return 5;
			case ConditionSet::ExpressionOp::EQ:
			case ConditionSet::ExpressionOp::NE:
			case ConditionSet::ExpressionOp::GT:
			case ConditionSet::ExpressionOp::LT:
			case ConditionSet::ExpressionOp::GE:
			case ConditionSet::ExpressionOp::LE:
				return 3;
			default:
				// Precedence for AND, OR, MAX and MIN and functions
				return 0;
		}
	}


	pair<ConditionSet::ExpressionOp, uint64_t> ConvertToken(const string &stringToken)
	{
		// First try to find the matching token.
		auto it = CS_TOKEN_CONVERSION.find(stringToken);
		if(it != CS_TOKEN_CONVERSION.end())
			return it->second;

		// Try number conversion, for if this is a number.
		if(DataNode::IsNumber(stringToken))
			return make_pair(ConditionSet::ExpressionOp::LIT, TERMINAL);

		// Try variable name conversion, for if this is a condition variable.
		if(DataNode::IsConditionName(stringToken))
			return make_pair(ConditionSet::ExpressionOp::VAR, TERMINAL);

		// If nothing matches, then we get the default INVALID value.
		return make_pair(ConditionSet::ExpressionOp::INVALID, 0);
	}
}



ConditionSet::ConditionSet(const ConditionsStore *conditions)
{
	this->conditions = conditions;
}



// Construct and Load() at the same time.
ConditionSet::ConditionSet(const DataNode &node, const ConditionsStore *conditions)
{
	Load(node, conditions);
}



// Construct a terminal with a literal value;
ConditionSet::ConditionSet(int64_t newLiteral, const ConditionsStore *conditions)
{
	expressionOperator = ExpressionOp::LIT;
	literal = newLiteral;
	this->conditions = conditions;
}



ConditionSet &ConditionSet::operator=(const ConditionSet &&other) noexcept
{
	// Guard against self-assignment as per C++ conventions.
	if(this == &other)
		return *this;

	// The other ConditionSet might be a child of the current one, so we
	// need to keep the children safe until the end of the assignment.
	// The attribute tells the compiler that oldChildren are actually used.
	[[maybe_unused]] vector<ConditionSet> oldChildren = std::move(children);

	// Then move over all content.
	expressionOperator = other.expressionOperator;
	literal = other.literal;
	conditionName = std::move(other.conditionName);
	children = std::move(other.children);
	conditions = other.conditions;

	return *this;
}



ConditionSet &ConditionSet::operator=(const ConditionSet &other)
{
	// Guard against self-assignment as per C++ conventions.
	if(this == &other)
		return *this;

	// The other ConditionSet might be a child of the current one, so we
	// need to keep the children safe until the end of the assignment.
	// The attribute tells the compiler that oldChildren are actually used.
	[[maybe_unused]] vector<ConditionSet> oldChildren = std::move(children);

	// Then copy over all content.
	expressionOperator = other.expressionOperator;
	literal = other.literal;
	conditionName = other.conditionName;
	children = other.children;
	conditions = other.conditions;

	return *this;
}



// Load a set of conditions from the children of this node.
void ConditionSet::Load(const DataNode &node, const ConditionsStore *conditions)
{
	if(!conditions)
		throw runtime_error("Unable to Load ConditionSet without a pointer to a ConditionsStore!");
	this->conditions = conditions;

	// The top-node is always an 'and' node, without the keyword.
	expressionOperator = ExpressionOp::AND;
	ParseChildren(node);
}



void ConditionSet::Save(DataWriter &out) const
{
	// Default should be AND, so if it is, then just write the subsets.
	// If this condition got optimized beyond AND, then re-add the AND by writing the current condition in full.
	if(expressionOperator == ExpressionOp::AND)
		for(const auto &child : children)
		{
			child.SaveAsTree(out);
			out.Write();
		}
	else
		SaveAsTree(out);
}



void ConditionSet::SaveAsTree(DataWriter &out) const
{
	auto it = find_if(CS_TOKEN_CONVERSION.begin(), CS_TOKEN_CONVERSION.end(),
		[this](const std::pair<const string, pair<ConditionSet::ExpressionOp, uint64_t>> &e) {
			return e.second.first == expressionOperator;
		});

	if(it != CS_TOKEN_CONVERSION.end() && (it->second.second & SINGLE_PARENT))
	{
		// Single parents get written as keyword, with the children below them.
		string opTxt = it->first;

		out.Write(opTxt);
		out.BeginChild();
		for(const auto &child : children)
		{
			child.SaveAsTree(out);
			out.Write();
		}
		out.EndChild();
	}
	else
	{
		// Anything else gets written inline.
		SaveInline(out);
	}
}



// Save a subset of conditions, by writing out tokens (without a newline).
void ConditionSet::SaveInline(DataWriter &out) const
{
	// Handle terminals and invalid.
	switch(expressionOperator)
	{
	case ExpressionOp::INVALID:
		out.WriteToken("never");
		return;
	case ExpressionOp::VAR:
		out.WriteToken(conditionName);
		return;
	case ExpressionOp::LIT:
		out.WriteToken(literal);
		return;
	default:
		break;
	}

	// Lookup the operator to get the operator text and parsing/writing style
	auto it = find_if(CS_TOKEN_CONVERSION.begin(), CS_TOKEN_CONVERSION.end(),
		[this](const std::pair<const string, pair<ConditionSet::ExpressionOp, uint64_t>> &e) {
			return e.second.first == expressionOperator;
		});
	if(it == CS_TOKEN_CONVERSION.end())
	{
		out.WriteToken("never");
		return;
	}
	string opTxt = it->first;

	// Handle infix operators.
	if((it->second.second & INFIX_OPERATOR) && !children.empty())
	{
		// Just write brackets so that we don't need to calculate precedence.
		// Children write their own brackets if they are also infix operators.
		if(children.size() > 1)
			out.WriteToken("(");
		children[0].SaveInline(out);
		for(unsigned int i = 1; i < children.size(); ++i)
		{
			out.WriteToken(opTxt);
			children[i].SaveInline(out);
		}
		if(children.size() > 1)
			out.WriteToken(")");
		return;
	}

	// Handle function operators.
	if((it->second.second & FUNCTION_OPERATOR) && !children.empty())
	{
		out.WriteToken(opTxt);
		out.WriteToken("(");
		children[0].SaveInline(out);
		for(unsigned int i = 1; i < children.size(); ++i)
		{
			out.WriteToken(",");
			children[i].SaveInline(out);
		}
		out.WriteToken(")");
		return;
	}

	out.WriteToken("never");
}



void ConditionSet::MakeNever()
{
	children.clear();
	expressionOperator = ExpressionOp::LIT;
	literal = 0;
}



// Check if there are any entries in this set.
// Invalid ConditionSets are also considered empty.
bool ConditionSet::IsEmpty() const
{
	// AND is the default toplevel operator for any condition, so whenever we encounter AND without any children
	// then there was nothing under the toplevel to parse, thus the condition was empty.
	return
		(expressionOperator == ExpressionOp::AND && children.size() == 0) ||
		(expressionOperator == ExpressionOp::INVALID);
}



// Check if the conditionset contains valid data
bool ConditionSet::IsValid() const
{
	return expressionOperator != ExpressionOp::INVALID;
}



bool ConditionSet::Test() const
{
	return Evaluate();
}



int64_t ConditionSet::Evaluate() const
{
	switch(expressionOperator)
	{
		case ExpressionOp::VAR:
		{
			if(!conditions)
				throw runtime_error("Unable to Evaluate ExpressionOp::VAR with condition name \"" + conditionName
					+ "\" in ConditionSet without a pointer to a ConditionsStore!");
			return conditions->Get(conditionName);
		}
		case ExpressionOp::LIT:
			return literal;
		case ExpressionOp::AND:
		{
			// An empty AND section returns true.
			if(children.empty())
				return 1;

			int64_t result = 0;
			for(const ConditionSet &child : children)
			{
				int64_t childResult = child.Evaluate();
				if(!childResult)
					return 0;
				// Assign the first non-zero result to the result variable.
				if(!result)
					result = childResult;
			}
			return result;
		}
		case ExpressionOp::OR:
			for(const ConditionSet &child : children)
			{
				int64_t childResult = child.Evaluate();
				// Return the first non-zero result.
				if(childResult)
					return childResult;
			}
			return 0;
		default:
			break;
	}

	// If we have an accumulator function and children, then let's use the accumulator on the children.
	// MAX and MIN are also handled by the accumulator.
	BinFun accumulatorOp = Op(expressionOperator);
	if(accumulatorOp != nullptr && !children.empty())
		return accumulate(next(children.begin()), children.end(), children[0].Evaluate(),
			[&accumulatorOp](int64_t accumulated, const ConditionSet &b) -> int64_t {
				return accumulatorOp(accumulated, b.Evaluate());
		});

	// If we don't have an accumulator function, or no children, then return the default value.
	return 0;
}



set<string> ConditionSet::RelevantConditions() const
{
	set<string> result;
	// Add the name from this set, if it is a VAR type operator.
	if(expressionOperator == ExpressionOp::VAR)
		result.emplace(conditionName);
	// Add the names from the children.
	for(const auto &child : children)
		for(const auto &rc : child.RelevantConditions())
			result.emplace(rc);
	return result;
}



bool ConditionSet::ParseFromStart(const DataNode &node)
{
	if(!conditions)
		throw runtime_error("Unable to ParseFromStart(full) for a ConditionSet without a pointer to a ConditionsStore!");

	const string &key = node.Token(0);
	// Special handling for 'and' and 'or' nodes.
	if(node.Size() == 1)
	{
		if(key == "and")
		{
			expressionOperator = ExpressionOp::AND;
			return ParseChildren(node);
		}
		if(key == "or")
		{
			expressionOperator = ExpressionOp::OR;
			return ParseChildren(node);
		}
		if(node.Token(0) == "min")
		{
			expressionOperator = ExpressionOp::MIN;
			return ParseChildren(node);
		}
		if(node.Token(0) == "max")
		{
			expressionOperator = ExpressionOp::MAX;
			return ParseChildren(node);
		}
	}

	// Nodes beyond this point should not have children.
	if(node.HasChildren())
		return FailParse(node, "Unexpected child-nodes under toplevel");

	// Special handling for 'never', 'has' and 'not' nodes.
	if(key == "never")
	{
		if(node.Size() > 1)
			return FailParse(node, "Tokens found after never keyword");

		expressionOperator = ExpressionOp::LIT;
		literal = 0;
		return true;
	}
	if(key == "has")
	{
		if(node.Size() != 2 || !DataNode::IsConditionName(node.Token(1)))
			return FailParse(node, "Has keyword requires a single condition");

		// Convert has keyword directly to the variable.
		expressionOperator = ExpressionOp::VAR;
		conditionName = node.Token(1);
		return true;
	}
	if(key == "not")
	{
		if(node.Size() != 2 || !DataNode::IsConditionName(node.Token(1)))
			return FailParse(node, "Not keyword requires a single condition");

		// Create `conditionName == 0` expression.
		expressionOperator = ExpressionOp::EQ;
		children.emplace_back(conditions);
		children.back().expressionOperator = ExpressionOp::VAR;
		children.back().conditionName = node.Token(1);
		children.emplace_back(0, conditions);
		return true;
	}

	int tokenNr = 0;
	if(!ParseNode(node, tokenNr))
		return false;

	return Optimize(node);
}



bool ConditionSet::ParseNode(const DataNode &node, int &tokenNr)
{
	if(!conditions)
		throw runtime_error("Unable to ParseNode(indexed) for a ConditionSet without a pointer to a ConditionsStore!");

	// Nodes beyond this point should not have children.
	if(node.HasChildren())
		return FailParse(node, "Unexpected child-nodes under arithmetic expression");

	if(!ParseGreedy(node, tokenNr))
		return FailParse();

	// Parsing from infix should have consumed and parsed all tokens.
	if(tokenNr < node.Size())
		return FailParse(node, "Tokens found after parsing full expression");

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
		case ExpressionOp::AND:
		case ExpressionOp::OR:
			// If we only have a single element, then replace the current OP/AND by its child.
			if(children.size() == 1)
				*this = children[0];

			break;

		case ExpressionOp::EQ:
		case ExpressionOp::NE:
		case ExpressionOp::LE:
		case ExpressionOp::GE:
		case ExpressionOp::LT:
		case ExpressionOp::GT:
			// TODO: Optimize boolean equality operators.
			break;

		case ExpressionOp::ADD:
		case ExpressionOp::SUB:
		case ExpressionOp::MUL:
		case ExpressionOp::DIV:
		case ExpressionOp::MOD:
		case ExpressionOp::MAX:
		case ExpressionOp::MIN:
			// TODO: Optimize arithmetic operators.
			break;

		case ExpressionOp::LIT:
		case ExpressionOp::VAR:
		case ExpressionOp::INVALID:
			break;
	}
	return returnValue;
}



bool ConditionSet::ParseChildren(const DataNode &node)
{
	if(!conditions)
		throw runtime_error("Unable to ParseBooleans in a ConditionSet without a pointer to a ConditionsStore!");

	if(!node.HasChildren())
		return FailParse(node, "Child-nodes expected, found none");

	// Load all child nodes.
	for(const DataNode &child : node)
	{
		children.emplace_back(conditions);
		children.back().ParseFromStart(child);

		if(children.back().expressionOperator == ExpressionOp::INVALID)
			return FailParse();
	}

	return true;
}



bool ConditionSet::ParseMini(const DataNode &node, int &tokenNr)
{
	if(!conditions)
		throw runtime_error("Unable to ParseMini in a ConditionSet without a pointer to a ConditionsStore!");

	if(tokenNr >= node.Size())
		return FailParse(node, "Expected terminal or sub-expression, found none");

	// Any (sub)expression should start with one of the following:
	// - an opening bracket.
	// - a literal number terminal.
	// - a condition name terminal.
	// - has keyword (but this is already handled at a higher level)
	// - not keyword (but this is already handled at a higher level)
	// - start of a function

	// If we have an open bracket, then we greedily parse everything until the closing bracket.
	if(node.Token(tokenNr) == "(")
	{
		++tokenNr;
		if(!ParseGreedy(node, tokenNr))
			return FailParse();
		if(tokenNr >= node.Size() || node.Token(tokenNr) != ")")
			return FailParse(node, "Missing closing bracket");

		// Make sure that this bracketed section gets used as a single terminal.
		if(!PushDownFull(node))
			return FailParse();

		// When we parsed upto the closing bracket, then we are done.
		++tokenNr;
		return true;
	}

	pair<ExpressionOp, uint64_t> opAndType = ConvertToken(node.Token(tokenNr));
	expressionOperator = opAndType.first;

	// If we have a function operator, then parse the function.
	if(opAndType.second & FUNCTION_OPERATOR)
	{
		++tokenNr;
		return ParseFunctionBody(node, tokenNr);
	}

	// Try to parse terminals (no need for checking if they are terminals.
	switch(expressionOperator)
	{
	case ExpressionOp::LIT:
		literal = node.Value(tokenNr);
		++tokenNr;
		break;
	case ExpressionOp::VAR:
		conditionName = node.Token(tokenNr);
		++tokenNr;
		break;
	default:
		return FailParse(node, "Expected terminal or open-bracket");
	}

	return true;
}



bool ConditionSet::ParseGreedy(const DataNode &node, int &tokenNr)
{
	// Any greedy parsing starts with parsing the minimal that we can parse.
	if(!ParseMini(node, tokenNr))
		return FailParse();

	// If we are at the end here, then we are done
	if(tokenNr >= node.Size())
		return true;

	// At this point we either have:
	// - a closing bracket
	// - a comma (when parsing a function body)
	// - an infix operator
	//
	// Handle the closing brackets and comma's.
	if(node.Token(tokenNr) == ")" || node.Token(tokenNr) == ",")
		return true;

	// If there are more tokens, then we need to have an infix operator here.
	// Give a precedence lower than anything existing, since we are not infix-parsing yet.
	if(!ParseFromInfix(node, tokenNr, -1))
		return FailParse();

	return true;
}



bool ConditionSet::ParseFunctionBody(const DataNode &node, int &tokenNr)
{
	if(!conditions)
		throw runtime_error("Unable to ParseFunction in a ConditionSet without a pointer to a ConditionsStore!");

	if(tokenNr >= node.Size())
		return FailParse(node, "Expected function body, found none");

	// Any function body should start with an opening bracket
	if(node.Token(tokenNr) != "(")
		return FailParse(node, "Expected function body, but missing opening bracket");

	++tokenNr;
	children.emplace_back(conditions);
	if(!children.back().ParseGreedy(node, tokenNr))
		return FailParse();

	while(node.Token(tokenNr) != ")")
	{
		if(node.Token(tokenNr) != ",")
			return FailParse(node, "Expected function terminator, or separator");

		++tokenNr;
		children.emplace_back(conditions);
		if(!children.back().ParseGreedy(node, tokenNr))
			return FailParse();
	}
	++tokenNr;

	return true;
}



bool ConditionSet::ParseFromInfix(const DataNode &node, int &tokenNr, int parentPrecedence)
{
	if(!conditions)
		throw runtime_error("Unable to ParseFromInfix in a ConditionSet without a pointer to a ConditionsStore!");

	// Keep on parsing until we reach an end-state (error, end-of-tokens, closing-bracket, lower precedence token)
	while(true)
	{
		// At this point, we can expect one of the following:
		// - an infix-operator
		// - a closing bracket (hopefully matching an earlier open bracket)
		// - an comma (hopefully being part of a function body)
		// - end of the tokens.

		// Reaching the end is fine, since we should have parsed a full terminal before this one.
		// Reaching a closing bracket also means we are done (the parent should handle it).
		if(tokenNr >= node.Size() || node.Token(tokenNr) == ")" || node.Token(tokenNr) == ",")
			return true;

		// Consume token and process it.
		pair<ExpressionOp, uint64_t> infixOp = ConvertToken(node.Token(tokenNr));
		if(!(infixOp.second & INFIX_OPERATOR))
			return FailParse(node, "Expected infix operator instead of \"" + node.Token(tokenNr) + "\"");

		if(tokenNr + 1 >= node.Size())
			return FailParse(node, "Expected terminal after infix operator \"" + node.Token(tokenNr) + "\"");

		// If the precedence of the new operator is less or equal than the parents operator, then let the parent handle it.
		if(Precedence(infixOp.first) <= parentPrecedence)
			return true;

		// If the precedence of the new operator is higher than the current operator, then parse the next
		// terminal into a new sub-expression.
		if((children.size() > 1) && (Precedence(expressionOperator) < Precedence(infixOp.first)))
		{
			if(!PushDownLast(node))
				return FailParse();
			if(!children.back().ParseFromInfix(node, tokenNr, Precedence(expressionOperator)))
				return FailParse();
			// The parser for the sub-expression handled everything with higher precedence. Start the loop over
			// to check what this parser needs to do next.
			continue;
		}

		// If the expression currently contains a terminal, then push it down.
		// Also push down the current expression if it has a higher or equal precedence to the new operator.
		if((children.size() == 0) || (children.size() > 1 && infixOp.first != expressionOperator &&
				Precedence(expressionOperator) >= Precedence(infixOp.first)))
			if(!PushDownFull(node))
				return FailParse();

		// If this expression contains only a single sub-expression, then we can apply the operator directly.
		if(children.size() == 1)
			expressionOperator = infixOp.first;

		// If we get the same operator as that we had, then let's just process it and continue the loop.
		if(infixOp.first == expressionOperator)
		{
			++tokenNr;
			if(!((children.emplace_back(conditions)).ParseMini(node, tokenNr)))
				return FailParse();

			continue;
		}
		return FailParse(node, "Precedence confusion on infix operator");
	}
}



bool ConditionSet::PushDownFull(const DataNode &node)
{
	ConditionSet ce(*this);
	children.clear();
	children.push_back(std::move(ce));
	expressionOperator = ExpressionOp::AND;

	return true;
}



bool ConditionSet::PushDownLast(const DataNode &node)
{
	// Can only perform push-down if there is at least one expression to push down.
	if(children.empty())
		return FailParse(node, "Cannot create sub-expression from void");

	// Store and remove the child that we want to push down.
	ConditionSet ce = std::move(children.back());
	children.pop_back();

	// Create a new last child.
	children.emplace_back(conditions);

	// Let the earlier removed child become a grandChild.
	children.back().children.push_back(std::move(ce));
	return true;
}



bool ConditionSet::FailParse()
{
	expressionOperator = ExpressionOp::INVALID;
	children.clear();
	return false;
}



bool ConditionSet::FailParse(const DataNode &node, const string &failText)
{
	node.PrintTrace(failText + ":");
	return FailParse();
}
