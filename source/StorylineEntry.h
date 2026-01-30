/* Storyline.h
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

#pragma once

#include "BookEntry.h"
#include "ConditionSet.h"

#include <map>
#include <set>
#include <string>

class DataNode;
class PlayerInfo;
class System;


// A representation of a storyline of missions. Storylines can be broken down into books, arcs, and chapters.
// Storylines have books, books have arcs, and arcs have chapters.
// Each component of a storyline can have a log book entry, conditions marking its start and end, and systems
// related to that component of the storyline to be marked on the map.
class StorylineEntry {
public:
	enum class Level {
		STORYLINE = 0,
		BOOK,
		ARC,
		CHAPTER,
	};


public:
	StorylineEntry() = default;
	void Load(const DataNode &node, const ConditionsStore *playerConditions, Level level = Level::STORYLINE);

	Level GetLevel() const;
	const std::string &TrueName() const;
	const std::string &DisplayName() const;
	const BookEntry &GetBookEntry() const;
	const std::set<const System *> &MarkSystems() const;
	const std::set<const System *> &CircleSystems() const;
	bool IsStarted() const;
	bool IsComplete() const;
	const std::map<std::string, StorylineEntry> &Children() const;


private:
	Level level = Level::STORYLINE;

	std::string trueName;
	std::string displayName;
	BookEntry bookEntry;

	std::set<const System *> marks;
	std::set<const System *> circles;

	ConditionSet toStart;
	ConditionSet toComplete;

	std::map<std::string, StorylineEntry> children;
};
