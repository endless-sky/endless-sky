/* GameEvent.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_EVENT_H_
#define GAME_EVENT_H_

#include "ConditionSet.h"
#include "Date.h"

#include <list>

class DataNode;
class DataWriter;
class PlayerInfo;



class GameEvent {
public:
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;
	
	const Date &GetDate() const;
	void SetDate(const Date &date);
	
	void Apply(PlayerInfo &player);
	
	
private:
	Date date;
	ConditionSet conditionsToApply;
	std::list<DataNode> changes;
};



#endif
