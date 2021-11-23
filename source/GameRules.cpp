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

using namespace std;



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
		const string &token = child.Token(1);
		rules[key] = (token == "true") ? 1. : ((token == "false") ? 0. : child.Value(1));
		if(saveToDefault)
			defaultRules[key] = rules[key];
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
