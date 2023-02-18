/* DataNode.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "DataNode.h"

#include "ConditionsStore.h"
#include "Logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>

using namespace std;



// Construct a DataNode and remember what its parent is.
DataNode::DataNode(const DataNode *parent) noexcept(false)
	: parent(parent), store(parent ? parent->store : nullptr)
{
	// To avoid a lot of memory reallocation, have every node start out with
	// capacity for four tokens. This makes file loading slightly faster, at the
	// cost of DataFiles taking up a bit more memory.
	tokens.reserve(4);
}



// Copy constructor.
DataNode::DataNode(const DataNode &other)
	: children(other.children), tokens(other.tokens), lineNumber(other.lineNumber), store(other.store)
{
	Reparent();
}



// Copy assignment operator.
DataNode &DataNode::operator=(const DataNode &other)
{
	children = other.children;
	tokens = other.tokens;
	lineNumber = other.lineNumber;
	store = other.store;
	Reparent();
	return *this;
}



DataNode::DataNode(DataNode &&other) noexcept
	: children(std::move(other.children)), tokens(std::move(other.tokens)), lineNumber(std::move(other.lineNumber)),
	store(other.store)
{
	Reparent();
}



DataNode::DataNode(std::shared_ptr<ConditionsStore> store)
	: store(store)
{
}



DataNode &DataNode::operator=(DataNode &&other) noexcept
{
	children.swap(other.children);
	tokens.swap(other.tokens);
	lineNumber = std::move(other.lineNumber);
	store = other.store;
	Reparent();
	return *this;
}



// Get the number of tokens in this line of the data file.
int DataNode::Size() const noexcept
{
	return tokens.size();
}



// Get all tokens.
const vector<string> &DataNode::Tokens() const noexcept
{
	return tokens;
}



// Get the token with the given index. No bounds checking is done.
// DataFile loading guarantees index 0 always exists.
const string &DataNode::Token(int index) const
{
	return tokens[index];
}



// Convert the token with the given index to a numerical value.
double DataNode::Value(int index) const
{
	// Check for empty strings and out-of-bounds indices.
	if(static_cast<size_t>(index) >= tokens.size() || tokens[index].empty())
		PrintTrace("Error: Requested token index (" + to_string(index) + ") is out of bounds:");
	else if(!IsNumber(tokens[index]))
		PrintTrace("Error: Cannot convert value \"" + tokens[index] + "\" to a number:");
	else
		return Value(tokens[index]);

	return 0.;
}



// Static helper function for any class which needs to parse string -> number.
double DataNode::Value(const string &token)
{
	// Allowed format: "[+-]?[0-9]*[.]?[0-9]*([eE][+-]?[0-9]*)?".
	if(!IsNumber(token))
	{
		Logger::LogError("Cannot convert value \"" + token + "\" to a number.");
		return 0.;
	}
	const char *it = token.c_str();

	// Check for leading sign.
	double sign = (*it == '-') ? -1. : 1.;
	it += (*it == '-' || *it == '+');

	// Digits before the decimal point.
	int64_t value = 0;
	while(*it >= '0' && *it <= '9')
		value = (value * 10) + (*it++ - '0');

	// Digits after the decimal point (if any).
	int64_t power = 0;
	if(*it == '.')
	{
		++it;
		while(*it >= '0' && *it <= '9')
		{
			value = (value * 10) + (*it++ - '0');
			--power;
		}
	}

	// Exponent.
	if(*it == 'e' || *it == 'E')
	{
		++it;
		int64_t sign = (*it == '-') ? -1 : 1;
		it += (*it == '-' || *it == '+');

		int64_t exponent = 0;
		while(*it >= '0' && *it <= '9')
			exponent = (exponent * 10) + (*it++ - '0');

		power += sign * exponent;
	}

	// Compose the return value.
	return copysign(value * pow(10., power), sign);
}



// Check if the token at the given index is a number in a format that this
// class is able to parse.
bool DataNode::IsNumber(int index) const
{
	// Make sure this token exists and is not empty.
	if(static_cast<size_t>(index) >= tokens.size() || tokens[index].empty())
		return false;

	return IsNumber(tokens[index]);
}



bool DataNode::IsNumber(const string &token)
{
	bool hasDecimalPoint = false;
	bool hasExponent = false;
	bool isLeading = true;
	for(const char *it = token.c_str(); *it; ++it)
	{
		// If this is the start of the number or the exponent, it is allowed to
		// be a '-' or '+' sign.
		if(isLeading)
		{
			isLeading = false;
			if(*it == '-' || *it == '+')
				continue;
		}
		// If this is a decimal, it may or may not be allowed.
		if(*it == '.')
		{
			if(hasDecimalPoint || hasExponent)
				return false;
			hasDecimalPoint = true;
		}
		else if(*it == 'e' || *it == 'E')
		{
			if(hasExponent)
				return false;
			hasExponent = true;
			// At the start of an exponent, a '-' or '+' is allowed.
			isLeading = true;
		}
		else if(*it < '0' || *it > '9')
			return false;
	}
	return true;
}



// Check if this node has any children. If so, the iterator functions below
// can be used to access them.
bool DataNode::IsCondition(int index) const
{
	return IsCondition(tokens[index]);
}



// Convert the token at the given index to a boolean. This returns false
// and prints an error if the index is out of range or the token cannot
// be interpreted as a number.
bool DataNode::BoolValue(int index) const
{
	// Check for empty strings and out-of-bounds indices.
	if(static_cast<size_t>(index) >= tokens.size() || tokens[index].empty())
		PrintTrace("Error: Requested token index (" + to_string(index) + ") is out of bounds:");
	else if(!IsBool(tokens[index]))
		PrintTrace("Error: Cannot convert value \"" + tokens[index] + "\" to a boolean:");
	else
	{
		const string &token = tokens[index];
		return token == "true" || token == "1";
	}

	return false;
}



bool DataNode::IsCondition(const string &token)
{
	// A condition reference begins with a condition reference character,
	// but it must not be only one or two condition reference characters.
	// This is to avoid clashes with operators.
	//
	// Take CONDITION_CHAR = '&' as an example:
	//    &&    = not a valid Condition string
	//    &     = not a valid Condition string
	//    &x    = a valid Condition string
	//    &xyz  = a valid Condition string
	//    &&xyz = a valid Condition string

	return token.size() > 1 && token[0] == CONDITION_CHAR
		&& (token.size() > 2 || token[1] != CONDITION_CHAR);
}



// Check if the token at the given index is a boolean, i.e. "true"/"1" or "false"/"0"
// as a string.
bool DataNode::IsBool(int index) const
{
	// Make sure this token exists and is not empty.
	if(static_cast<size_t>(index) >= tokens.size() || tokens[index].empty())
		return false;

	return IsBool(tokens[index]);
}



bool DataNode::IsBool(const string &token)
{
	return token == "true" || token == "1" || token == "false" || token == "0";
}



// Check if this node has any children.
bool DataNode::HasChildren() const noexcept
{
	return !children.empty();
}



// Iterator to the beginning of the list of children.
list<DataNode>::const_iterator DataNode::begin() const noexcept
{
	return children.begin();
}



// Iterator to the end of the list of children.
list<DataNode>::const_iterator DataNode::end() const noexcept
{
	return children.end();
}



// Print a message followed by a "trace" of this node and its parents.
int DataNode::PrintTrace(const string &message) const
{
	if(!message.empty())
		Logger::LogError(message);

	// Recursively print all the parents of this node, so that the user can
	// trace it back to the right point in the file.
	size_t indent = 0;
	if(parent)
		indent = parent->PrintTrace() + 2;
	if(tokens.empty())
		return indent;

	// Convert this node back to tokenized text, with quotes used as necessary.
	string line = !parent ? "" : "L" + to_string(lineNumber) + ": ";
	line.append(string(indent, ' '));
	for(const string &token : tokens)
	{
		if(&token != &tokens.front())
			line += ' ';
		bool hasSpace = any_of(token.begin(), token.end(), [](char c) { return isspace(c); });
		bool hasQuote = any_of(token.begin(), token.end(), [](char c) { return (c == '"'); });
		if(hasSpace)
			line += hasQuote ? '`' : '"';
		line += token;
		if(hasSpace)
			line += hasQuote ? '`' : '"';
	}
	Logger::LogError(line);

	// Put an empty line in the log between each error message.
	if(!message.empty())
		Logger::LogError("");

	// Tell the caller what indentation level we're at now.
	return indent;
}



// Get the value, and if it was a variable, the variable name
Condition<double> DataNode::AsCondition(int index) const
{
	// Check for empty strings and out-of-bounds indices.
	if(static_cast<size_t>(index) >= tokens.size() || tokens[index].empty())
		PrintTrace("Error: Requested token index (" + to_string(index) + ") is out of bounds:");
	else if(IsNumber(tokens[index]))
		return Condition<double>(Value(index));
	else if(IsCondition(tokens[index]))
		return Condition<double>(store, tokens[index].substr(1));
	PrintTrace("Cannot convert value \"" + tokens[index] + "\" to a number or condition name. "
		"Condition names must begin with " + CONDITION_CHAR_NAME + ". "
		"This token will be replaced with the number 0.");
	return Condition<double>(0);
}



shared_ptr<ConditionsStore> DataNode::Store()
{
	return store;
}



void DataNode::SetStore(std::shared_ptr<ConditionsStore> newStore)
{
	store = newStore;
	for(DataNode &child : children)
		child.SetStore(newStore);
}



// Adjust the parent pointers when a copy is made of a DataNode.
void DataNode::Reparent() noexcept
{
	if(parent)
		store = parent->store;
	for(DataNode &child : children)
	{
		child.parent = this;
		child.Reparent();
	}
}
