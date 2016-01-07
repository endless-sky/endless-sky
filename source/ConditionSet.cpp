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

#include <cmath>

using namespace std;

namespace {
	typedef int (*BinFun)(int, int);
	BinFun Op(const string &op)
	{
		static const map<string, BinFun> opMap = {
			{"==", [](int a, int b) -> int { return a == b; }},
			{"!=", [](int a, int b) -> int { return a != b; }},
			{"<", [](int a, int b) -> int { return a < b; }},
			{">", [](int a, int b) -> int { return a > b; }},
			{"<=", [](int a, int b) -> int { return a <= b; }},
			{">=", [](int a, int b) -> int { return a >= b; }},
			{"=", [](int a, int b) { return b; }},
			{"+=", [](int a, int b) { return a + b; }},
			{"-=", [](int a, int b) { return a - b; }},
			{"<?=", [](int a, int b) { return min(a, b); }},
			{">?=", [](int a, int b) { return max(a, b); }}
		};
		
		auto it = opMap.find(op);
		return (it != opMap.end() ? it->second : nullptr);
	}
}



// Load a set of conditions from the children of this node.
void ConditionSet::Load(const DataNode &node)
{
	isOr = (node.Token(0) == "or");
	for(const DataNode &child : node)
		Add(child);
}



void ConditionSet::Save(DataWriter &out) const
{
	for(const Entry &entry : entries)
		out.Write(entry.name, entry.op, entry.value);
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



bool ConditionSet::IsEmpty() const
{
	return entries.empty() && children.empty();
}



void ConditionSet::Add(const DataNode &node)
{
	if(node.Size() == 2)
	{
		if(!Add(node.Token(0), node.Token(1)))
			node.PrintTrace("Unrecognized condition expression:");
	}
	else if(node.Size() == 3)
	{
		if(!Add(node.Token(0), node.Token(1), node.Value(2)))
			node.PrintTrace("Unrecognized condition expression:");
	}
	else if(node.Size() == 1 && node.Token(0) == "never")
		entries.emplace_back("", "!=", 0);
	else if(node.Size() == 1 && (node.Token(0) == "and" || node.Token(0) == "or"))
	{
		children.emplace_back();
		children.back().Load(node);
	}
	else
		node.PrintTrace("Unrecognized condition expression:");
}



bool ConditionSet::Add(const string &firstToken, const string &secondToken)
{
	if(firstToken == "not")
		entries.emplace_back(secondToken, "==", 0);
	else if(firstToken == "has")
		entries.emplace_back(secondToken, "!=", 0);
	else if(firstToken == "set")
		entries.emplace_back(secondToken, "=", 1);
	else if(firstToken == "clear")
		entries.emplace_back(secondToken, "=", 0);
	else if(secondToken == "++")
		entries.emplace_back(firstToken, "+=", 1);
	else if(secondToken == "--")
		entries.emplace_back(firstToken, "-=", 1);
	else
		return false;
	
	return true;
}



bool ConditionSet::Add(const string &name, const string &op, int value)
{
	BinFun fun = Op(op);
	if(!fun || isnan(value))
		return false;
	
	entries.emplace_back(name, op, value);
	return true;
}



bool ConditionSet::Test(const map<string, int> &conditions) const
{
	for(const Entry &entry : entries)
	{
		auto it = conditions.find(entry.name);
		bool result = entry.fun(it != conditions.end() ? it->second : 0, entry.value);
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



void ConditionSet::Apply(map<string, int> &conditions) const
{
	for(const Entry &entry : entries)
	{
		int &c = conditions[entry.name];
		c = entry.fun(c, entry.value);
	}
	for(const ConditionSet &child : children)
		child.Apply(conditions);
}



ConditionSet::Entry::Entry(const string &name, const string &op, int value)
	: name(name), op(op), fun(Op(op)), value(value)
{
}
