/* StartConditions.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef START_CONDITIONS_H_
#define START_CONDITIONS_H_

#include "CoreStartData.h"

#include "ConditionSet.h"
#include "Conversation.h"
#include "ExclusiveItem.h"

#include <string>
#include <vector>

class DataNode;
class Ship;
class Sprite;



class StartConditions : public CoreStartData {
public:
	enum class StartState : int {
		HIDDEN,
		VISIBLE,
		REVEALED,
		UNLOCKED
	};

	struct StartInfo {
		const Sprite *thumbnail = nullptr;
		std::string name;
		std::string description;
		std::string system;
		std::string planet;
	};

public:
	StartConditions() = default;
	explicit StartConditions(const DataNode &node);

	void Load(const DataNode &node);
	void LoadState(const DataNode &node, StartState state);
	// Finish loading the ship definitions.
	void FinishLoading();

	// A valid start scenario has a valid system, planet, and conversation.
	// Any ships given to the player must also be valid models.
	bool IsValid() const;

	const ConditionSet &GetConditions() const noexcept;
	const std::vector<Ship> &Ships() const noexcept;

	// Get this start's intro conversation.
	const Conversation &GetConversation() const;

	// Information needed for the scenario picker.
	const Sprite *GetThumbnail();
	const std::string &GetDisplayName();
	const std::string &GetDescription();
	const std::string &GetPlanetName();
	const std::string &GetSystemName();

	void SetState(const ConditionsStore &conditionsStore);
	StartConditions::StartState GetState() const;

	bool Visible(const ConditionsStore &conditionsStore) const;
	bool Revealed(const ConditionsStore &conditionsStore) const;
	bool Unlocked(const ConditionsStore &conditionsStore) const;


private:
	// Conditions that will be set for any pilot that begins with this scenario.
	ConditionSet conditions;
	// Ships that a new pilot begins with (rather than being required to purchase one).
	std::vector<Ship> ships;

	// The conversation to display when a game begins with this scenario.
	ExclusiveItem<Conversation> conversation;

	std::map<StartState, StartInfo> infoByState;
	StartState state = StartState::HIDDEN;

	ConditionSet toDisplay;
	ConditionSet toReveal;
	ConditionSet toUnlock;
};



#endif
