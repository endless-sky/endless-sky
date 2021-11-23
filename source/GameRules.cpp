/* GameRules.cpp
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameRules.h"

#include "DataNode.h"

#include <map>
#include <set>

using namespace std;

namespace {
	// Allowable rule names.
	const set<string> RULE_NAMES = {
		"depreciation: full",
		"depreciation: daily",
		"depreciation: grace period",
		"depreciation: max age",
		
		"flotsam: drag",
		"flotsam: commodity: base lifetime",
		"flotsam: commodity: random lifetime",
		"flotsam: outfit: base lifetime",
		"flotsam: outfit: cost scale",
		"flotsam: outfit: base cost",
		
		"hit force: base mass",
		"hit force: base scale",
		
		"person spawnrate",
	};
	
	// A mapping of gamerule constant names to specifically-allowed minimum values.
	// Based on the specific usage of the constant, the allowed minimum value is
	// chosen to avoid disallowed or undesirable behaviors (such as dividing by zero).
	// The default behavior (for those constant names not in this map) is a minimum
	// value of 0.
	const auto MINIMUM_ALLOWED = map<string, double>{
		{"person spawnrate", 1.},
	};
}



// Load a gamerules node.
void GameRules::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Skipping gamerule with no value:");
			continue;
		}
		
		const string &key = child.Token(0);
		
		if(!RULE_NAMES.count(key))
		{
			child.PrintTrace("Skipping unrecognized gamerule:");
			continue;
		}
		
		const string &token = child.Token(1);
		double value = (token == "true") ? 1. : ((token == "false") ? 0. : child.Value(1));
		double minValue = 0.;
		
		auto it = MINIMUM_ALLOWED.find(key);
		if(it != MINIMUM_ALLOWED.end())
			minValue = it->second;
		
		rules[key] = max(minValue, value);
	}
}



// Revert the rules of this GameRule to the rules of other.
void GameRules::Revert(const GameRules &other)
{
	for(const auto &rule : other.rules)
		rules[rule.first] = rule.second;
}



// Get a gamerule constant.
double GameRules::Get(const string &key) const
{
	return rules.Get(key);
}
