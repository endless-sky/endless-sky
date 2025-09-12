/* GameVersionConstraints.cpp
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GameVersionConstraints.h"

#include "DataNode.h"

using namespace std;



void GameVersionConstraints::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "min" && child.Size() >= 2)
			min = GameVersion{child.Token(1)};
		else if(key == "max" && child.Size() >= 2)
			max = GameVersion{child.Token(1)};
		else
			*this = GameVersionConstraints{GameVersion{key}};
	}
}



bool GameVersionConstraints::IsEmpty() const
{
	return !min.IsValid() && !max.IsValid();
}



bool GameVersionConstraints::Matches(const GameVersion &compare) const
{
	return (!min.IsValid() || compare >= min) && (!max.IsValid() || compare <= max);
}



string GameVersionConstraints::Description() const
{
	string text;
	if(!IsEmpty())
	{
		text += "  Game Version:\n";
		if(min.IsValid())
			text += "    Minimum: " + min.ToString() + '\n';
		if(max.IsValid())
			text += "    Maximum: " + max.ToString() + '\n';
	}
	return text;
}
