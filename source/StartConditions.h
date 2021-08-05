/* StartConditions.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef START_CONDITIONS_H_
#define START_CONDITIONS_H_

#include "CoreStartData.h"

#include "ConditionSet.h"
#include "Conversation.h"

#include <string>
#include <vector>

class DataNode;
class Ship;
class Sprite;



class StartConditions : public CoreStartData {
public:
	StartConditions() = default;
	explicit StartConditions(const DataNode &node);
	
	void Load(const DataNode &node);
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
	const Sprite *GetThumbnail() const noexcept;
	const std::string &GetDisplayName() const noexcept;
	const std::string &GetDescription() const noexcept;
	
	
private:
	// Conditions that will be set for any pilot that begins with this scenario.
	ConditionSet conditions;
	// Ships that a new pilot begins with (rather than being required to purchase one).
	std::vector<Ship> ships;
	
	// The conversation to display when a game begins with this scenario.
	Conversation conversation;
	const Conversation *stockConversation = nullptr;
	
	const Sprite *thumbnail = nullptr;
	// The user-friendly display name for this starting scenario.
	std::string name;
	std::string description;
};



#endif
