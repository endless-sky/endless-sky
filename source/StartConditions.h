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

#pragma once

#include "CoreStartData.h"

#include "ConditionSet.h"
#include "Conversation.h"
#include "Date.h"
#include "ExclusiveItem.h"

#include <string>
#include <vector>

class ConditionsStore;
class DataNode;
class Ship;
class Sprite;



class StartConditions : public CoreStartData {
public:
	// Various states that a StartConditions can be in depending on global conditions.
	enum class StartState : int {
		HIDDEN,
		DISPLAYED,
		REVEALED,
		UNLOCKED
	};

	// Information to be shown to the player depending on current StartState.
	// Information which is a child of the root node gets loaded into the UNLOCKED StartState.
	// Information under "on (display | reveal)" nodes gets loaded into their respective VISIBLE or REVEALED states.
	// The HIDDEN state has no information to display.
	class StartInfo {
	public:
		const Sprite *thumbnail = nullptr;
		std::string displayName;
		std::string description;
		// StartInfo stores the name of the provided system and planet instead of a pointer to them
		// so that the names don't need to be actual systems or planets in the game for the VISIBLE
		// and REVEALED states. The UNLOCKED state must have valid information, though.
		std::string system;
		std::string planet;

		Date date;
		std::string dateString;
		std::string credits;
		std::string debt;
	};


public:
	StartConditions() = default;
	explicit StartConditions(const DataNode &node, const ConditionsStore *globalConditions,
		const ConditionsStore *playerConditions);

	void Load(const DataNode &node, const ConditionsStore *globalConditions,
		const ConditionsStore *playerConditions);
	// Finish loading the ship definitions.
	void FinishLoading();

	// A valid start scenario has a valid system, planet, and conversation.
	// Any ships given to the player must also be valid models.
	bool IsValid() const;

	const ConditionAssignments &GetConditions() const noexcept;
	const std::vector<Ship> &Ships() const noexcept;

	// Get this start's intro conversation.
	const Conversation &GetConversation() const;

	// Information needed for the scenario picker.
	const Sprite *GetThumbnail() const noexcept;
	const std::string &GetDisplayName() const noexcept;
	const std::string &GetDescription() const noexcept;
	const std::string &GetPlanetName() const noexcept;
	const std::string &GetSystemName() const noexcept;
	const std::string &GetDateString() const noexcept;
	const std::string &GetCredits() const noexcept;
	const std::string &GetDebt() const noexcept;

	// Determine whether this StartConditions should be displayed to the player.
	bool Visible() const;
	// Set the current state of this StartConditions. This influences what
	// information from the above getters is returned.
	void SetState();
	bool IsUnlocked() const;


private:
	// Helper functions for loading StartInfo.
	void LoadState(const DataNode &node, StartState state);
	bool LoadStateChild(const DataNode &child, StartInfo &info, bool &clearDescription, bool isAdd);
	void FillState(StartState fillState, const Sprite *thumbnail);


private:
	// Conditions that will be set for any pilot that begins with this scenario.
	ConditionAssignments conditions;
	// Ships that a new pilot begins with (rather than being required to purchase one).
	std::vector<Ship> ships;

	// The conversation to display when a game begins with this scenario.
	ExclusiveItem<Conversation> conversation;

	// The current state of this StartConditions and the StartInfo to be used for each state.
	StartState state = StartState::HIDDEN;
	std::map<StartState, StartInfo> infoByState;

	// ConditionSets which determine the StartState of this StartConditions.
	ConditionSet toDisplay;
	ConditionSet toReveal;
	ConditionSet toUnlock;
};
