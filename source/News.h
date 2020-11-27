/* News.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef NEWS_H_
#define NEWS_H_

#include "ConditionSet.h"
#include "LocationFilter.h"
#include "Phrase.h"

#include <string>
#include <vector>

class DataNode;
class Planet;
class Sprite;



// This represents a person you can "talk to" in the spaceport to get some local
// news. One specification can contain many possible portraits and messages.
class News {
public:
	void Load(const DataNode &node);
	
	// Check whether this news item has anything to say.
	bool IsEmpty() const;
	// Check if this news item is available given the player's planet and conditions.
	bool Matches(const Planet *planet, const std::map<std::string, std::int64_t> &conditions) const;
	
	// Get the speaker's name.
	std::string Name() const;
	// Pick a portrait at random out of the possible options.
	const Sprite *Portrait() const;
	// Get the speaker's message, chosen randomly.
	std::string Message() const;
	
	
private:
	LocationFilter location;
	ConditionSet toShow;
	
	Phrase names;
	std::vector<const Sprite *> portraits;
	Phrase messages;
};

#endif
