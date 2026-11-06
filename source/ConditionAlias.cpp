/* ConditionAlias.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ConditionAlias.h"

#include "GameData.h"
#include "Logger.h"

#include <ranges>

using namespace std;



void ConditionAlias::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	name = node.Token(1);
	conditions.Load(node, playerConditions);
}



void ConditionAlias::FinishLoading()
{
	set<string> visited{name};
	if(LoopCheck(name, visited, *this))
		isValid = false;
}



bool ConditionAlias::Test() const
{
	return isValid ? conditions.Test() : false;
}



bool ConditionAlias::LoopCheck(const string &root, set<string> visited, const ConditionAlias &alias)
{
	set<string> usesConditions = alias.conditions.RelevantConditions();
	for(const auto &[otherName, otherAlias] : GameData::ConditionAliases())
	{
		if(!usesConditions.contains(otherName))
			continue;
		if(!visited.insert(otherName).second)
		{
			Logger::Log("Condition alias contains infinite loop beginning with alias \"" + root + "\".",
				Logger::Level::WARNING);
			return true;
		}
		if(LoopCheck(root, visited, otherAlias))
			return true;
	}
	return false;
}
