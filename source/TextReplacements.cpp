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

using namespace std;

TextReplacements::TextReplacements(const DataNode &node)
{
	Load(node);
}



void TextReplacements::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
			child.PrintTrace("Skipping improper substitution syntax:");
		else
		{
			ConditionSet toSubstitute;
			if(child.HasChildren())
				toSubstitute.Load(child);
			substitutions.emplace_back(child.Token(0), make_pair(toSubstitute, child.Token(1)));
		}
	}
}



// Get a map of text replacements.
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
