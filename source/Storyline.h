/* Storyline.h
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

#ifndef STORYLINE_H_
#define STORYLINE_H_

#include "ConditionSet.h"

#include <list>
#include <string>

class ConditionsStore;
class DataNode;
class PlayerInfo;



// Class representing a storyline: conditions to begin and end it, and to complete
// chapters along the way.
// Intended to help with discoverabiity of content and tracking player progress.
class Storyline {
public:
	// Static method to help with UIs.
	// Returns the names of the started, ended, and visible available
	// storylines, together with a count of the hidden available storylines. For
	// storylines which have chapters, the started text also includes the
	// progress.
	static void Progress(const PlayerInfo &player,
		std::vector<std::string> &started,
		std::vector<std::string> &ended,
		std::vector<std::string> &available,
		int &hidden);

	// Load a storyline's definition.
	void Load(const DataNode &node);

	bool IsValid() const;
	const std::string &Name() const;

	// Check whether each of the properties applies.
	bool HasStarted(const ConditionsStore &conditions) const;
	bool HasEnded(const ConditionsStore &conditions) const;
	bool IsBlocked(const ConditionsStore &conditions) const;

	// Return number of chapters.
	// This is always at least one, as the start in treated as a chapter.
	int ChapterCount() const;

	// Return the number of chapters started. If the storyline has not started,
	// this is zero, otherwise it is the number of chapter conditions which are
	// met plus 1 for the start condition.
	int ChaptersStarted(const ConditionsStore &conditions) const;

	// Return true if this storyline is visible/hidden.
	bool IsVisible() const;
	bool IsHidden() const;

private:
	enum class Visibility : int {
		// Visibility was not explicitly set. Currently equivalent to hidden,
		// but kept separate to allow other options in future.
		unset = 0,
		// Name is displayed in available.
		visible,
		// Name is not displayed in available, but included in the count.
		hidden,
		// No information is displayed in available.
		secret
	};

	bool isDefined = false;
	std::string name;

	ConditionSet startCondition;
	ConditionSet endCondition;
	std::list<ConditionSet> chapterConditions;
	ConditionSet blockedCondition;
	Visibility visibility = Visibility::unset;
};

#endif
