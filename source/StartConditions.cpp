/* StartConditions.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StartConditions.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "GameData.h"
#include "Planet.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "System.h"

using namespace std;



StartConditions::StartConditions(const DataNode &node)
{
	Load(node);
}



void StartConditions::Load(const DataNode &node)
{
	identifier = (node.Size() >= 2) ? node.Token(1) : "(Unnamed Start)";
	for(const DataNode &child : node)
	{
		if(CoreStartData::LoadChild(child))
		{
			// This child node contained core information and was successfully parsed.
		}
		else if(child.Token(0) == "name" && child.Size() >= 2)
			name = child.Token(1);
		else if(child.Token(0) == "description" && child.Size() >= 2)
			description += child.Token(1) + "\n";
		else if(child.Token(0) == "thumbnail" && child.Size() >= 2)
			thumbnail = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "ship" && child.Size() >= 2)
		{
			// TODO: support named stock ships.
			// Assume that child nodes introduce a full ship definition. Even without child nodes,
			// Ship::Load + Ship::FinishLoading will create the expected ship instance if there is
			// a 3rd token (i.e. this will be treated as though it were a ship variant definition,
			// without making the variant available to the rest of GameData).
			if(child.HasChildren() || child.Size() >= 3)
				ships.emplace_back(child);
			// If there's only 2 tokens & there's no child nodes, the created instance would be ill-formed.
			else
				child.PrintTrace("Skipping unsupported use of a \"stock\" ship (a full definition is required):");
		}
		else if(child.Token(0) == "conversation" && child.HasChildren())
			conversation.Load(child);
		else if(child.Token(0) == "conversation" && child.Size() >= 2)
			stockConversation = GameData::Conversations().Get(child.Token(1));
		else
			conditions.Add(child);
	}
	if(description.empty())
		description = "(No description provided.)";
}
 


// Finish loading the ship definitions.
void StartConditions::FinishLoading()
{
	for(Ship &ship : ships)
		ship.FinishLoading(true);
	
	if(!GetConversation().IsValidIntro())
		Files::LogError("Warning: The start scenario \"" + name + "\" has an invalid starting conversation.");
}



bool StartConditions::IsValid() const
{	// A start must specify a valid system.
	if(!system || !system->IsValid())
		return false;
	
	// A start must specify a valid planet in its specified system.
	if(!planet || !planet->IsValid() || planet->GetSystem() != system)
		return false;
	
	// A start must reference a valid "intro" conversation, either stock or custom.
	if((stockConversation && !stockConversation->IsValidIntro()) || (!stockConversation && !conversation.IsValidIntro()))
		return false;
	
	// All ship models must be valid.
	if(any_of(ships.begin(), ships.end(), [](const Ship &it) noexcept -> bool { return !it.IsValid(); }))
		return false;
	
	return true;
}



const ConditionSet &StartConditions::GetConditions() const noexcept
{
	return conditions;
}



const vector<Ship> &StartConditions::Ships() const noexcept
{
	return ships;
}



const Conversation &StartConditions::GetConversation() const
{
	return stockConversation ? *stockConversation : conversation;
}



const Sprite *StartConditions::GetThumbnail() const noexcept
{
	return thumbnail;
}



const std::string &StartConditions::GetDisplayName() const noexcept
{
	return name;
}



const std::string &StartConditions::GetDescription() const noexcept
{
	return description;
}
