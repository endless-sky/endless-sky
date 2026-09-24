/* StorylineEntry.cpp
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

#include "StorylineEntry.h"

#include "ConditionSet.h"
#include "DataNode.h"
#include "PlayerInfo.h"

using namespace std;



void StorylineEntry::Load(const DataNode &node, const ConditionsStore *playerConditions, Level level)
{
	if(node.Size() < 2)
		return;
	trueName = node.Token(1);
	this->level = level;

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "section name" && hasValue && (level == Level::STORYLINE || level == Level::BOOK))
			sectionName = child.Token(1);
		else if(key == "heading" && hasValue)
			heading = child.Token(1);
		else if(key == "log")
			initialEntry.Load(child, 1);
		else if(key == "complete log")
			completeEntry.Load(child, 1);
		else if(key == "to" && hasValue)
		{
			const string &value = child.Token(1);
			if(value == "start")
				toStart.Load(child, playerConditions);
			else if(value == "complete")
				toComplete.Load(child, playerConditions);
		}
		else if(level == Level::STORYLINE && key == "book" && hasValue)
			children[child.Token(1)].Load(child, playerConditions, Level::BOOK);
		else if(level == Level::BOOK && key == "arc" && hasValue)
			children[child.Token(1)].Load(child, playerConditions, Level::ARC);
		else if(level == Level::ARC && key == "chapter" && hasValue)
			children[child.Token(1)].Load(child, playerConditions, Level::CHAPTER);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



StorylineEntry::Level StorylineEntry::GetLevel() const
{
	return level;
}



const string &StorylineEntry::TrueName() const
{
	return trueName;
}



const string &StorylineEntry::SectionName() const
{
	return sectionName.empty() ? trueName : sectionName;
}



const string &StorylineEntry::Heading() const
{
	return heading.empty() ? SectionName() : heading;
}



const BookEntry &StorylineEntry::InitialEntry() const
{
	return initialEntry;
}



const BookEntry &StorylineEntry::CompleteEntry() const
{
	return completeEntry;
}



bool StorylineEntry::IsStarted() const
{
	return toStart.Test();
}



bool StorylineEntry::IsComplete() const
{
	return toComplete.Test();
}



const map<string, StorylineEntry> &StorylineEntry::Children() const
{
	return children;
}
