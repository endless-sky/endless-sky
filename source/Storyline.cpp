/* Storyline.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Storyline.h"

#include "ConditionSet.h"
#include "ConditionsStore.h"
#include "DataNode.h"
#include "GameData.h"
#include "PlayerInfo.h"
#include "Set.h"

using namespace std;



int Storyline::Progress(const PlayerInfo &player,
	vector<string> &started,
	vector<string> &ended,
	vector<string> &available)
{
	int hidden = 0;
	const ConditionsStore &playerConditions = player.Conditions();
	for(const auto &s : GameData::Storylines())
	{
		const Storyline &storyline = s.second;
		const string &name = storyline.Name();
		if(storyline.HasEnded(playerConditions))
			ended.emplace_back(name);
		else if(storyline.HasStarted(playerConditions))
		{
			int chapterCount = storyline.ChapterCount();
			if(chapterCount == 1)  // 1 means there is only a start.
				started.emplace_back(name);
			else
			{
				started.emplace_back(
					name+" (chapter "+
					to_string(storyline.ChaptersStarted(playerConditions))+
					" of "+to_string(chapterCount)+")");
			}
		}
		else if(!storyline.IsBlocked(playerConditions))
		{
			if(storyline.IsVisible())
				available.emplace_back(name);
			else if(storyline.IsHidden())
				hidden += 1;
		}
	}
	return hidden;
}



void Storyline::Load(const DataNode &node)
{
	if(node.Size() < 2) {
		node.PrintTrace("Storyline is missing a name");
		return;
	}
	name = node.Token(1);
	isDefined = true;

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "start")
			startCondition.Load(child);
		else if(child.Token(0) == "end")
			endCondition.Load(child);
		else if(child.Token(0) == "chapter")
			chapterConditions.emplace_back(child);
		else if(child.Token(0) == "blocked")
			blockedCondition.Load(child);
		else if(child.Token(0) == "visibility" && child.Size() >= 2)
		{
			if(child.Token(1) == "visible")
				visibility = Visibility::visible;
			else if(child.Token(1) == "hidden")
				visibility = Visibility::hidden;
			else if(child.Token(1) == "secret")
				visibility = Visibility::secret;
			else
				child.PrintTrace("Unknown visibility: "+child.Token(1)+
					" on "+name);
		}
		else
			child.PrintTrace("Skipping unrecognized attribute: "+
				child.Token(0)+": "+to_string(child.Size()));
	}
}



bool Storyline::IsValid() const
{
	return isDefined;
}



const string &Storyline::Name() const
{
	return name;
}



// In each of the following we return
// !condition.IsEmpty && condition.Test
// as an empty condition always evaluates to true.
bool Storyline::HasStarted(const ConditionsStore &conditions) const
{
	return !startCondition.IsEmpty() && startCondition.Test(conditions);
}



bool Storyline::HasEnded(const ConditionsStore &conditions) const
{
	return !endCondition.IsEmpty() && endCondition.Test(conditions);
}



bool Storyline::IsBlocked(const ConditionsStore &conditions) const
{
	return !blockedCondition.IsEmpty() && blockedCondition.Test(conditions);
}



int Storyline::ChapterCount() const
{
	return chapterConditions.size() + 1;
}



int Storyline::ChaptersStarted(const ConditionsStore &conditions) const
{
	if(!HasStarted(conditions)) return 0;
	int started = 1;
	for(const ConditionSet &c : chapterConditions) {
		if(!c.IsEmpty() && c.Test(conditions))
			started++;
	}
	return started;
}



bool Storyline::IsVisible() const
{
	return visibility == Visibility::visible;
}



bool Storyline::IsHidden() const
{
	// Consider unset to be the same as hidden.
	return visibility == Visibility::hidden || visibility == Visibility::unset;
}
