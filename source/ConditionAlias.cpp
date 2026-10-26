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
	conditions.Load(node, playerConditions);
}



void ConditionAlias::FinishLoading()
{
	set<string> usesConditions = conditions.RelevantConditions();
	for(const string &otherName : GameData::ConditionAliases() | views::keys)
	{
		if(usesConditions.contains(otherName))
		{
			Logger::Log("Condition alias \"" + name + "\" will be disabled, as it contains the alias \"" + otherName
				+ "\". Condition aliases may not contain other condition aliases.", Logger::Level::WARNING);
			isValid = false;
			return;
		}
	}
}



bool ConditionAlias::Test() const
{
	return isValid ? conditions.Test() : false;
}
