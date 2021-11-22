/* TextReplacements.cpp
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TextReplacements.h"

#include "ConditionSet.h"
#include "DataNode.h"
#include "PlayerInfo.h"

#include <set>

using namespace std;



void TextReplacements::Load(const DataNode &node)
{
	// Check for reserved keys. Only some hardcoded replacement keys are
	// reserved, as these ones are done on the fly after all other replacements
	// have been done.
	const set<string> reserved = {"<first>", "<last>", "<ship>"};
	
	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Skipping improper substitution syntax:");
			continue;
		}
		
		const string &key = child.Token(0);
		if(reserved.count(key))
		{
			child.PrintTrace("Skipping reserved substitution key \"" + key + "\":");
			continue;
		}
		
		ConditionSet toSubstitute(child);
		substitutions.emplace_back(key, make_pair(std::move(toSubstitute), child.Token(1)));
	}
}



// Get a map of text replacements after evaluating all possible replacements.
map<string, string> TextReplacements::Substitutions(const PlayerInfo &player) const
{
	map<string, string> subs;
	for(const auto &sub : substitutions)
	{
		const string &key = sub.first;
		const ConditionSet &toSub = sub.second.first;
		const string &replacement = sub.second.second;
		if(toSub.Test(player.Conditions()))
			subs[key] = replacement;
	}
	return subs;
}
