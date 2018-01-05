/* Government.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Government.h"

#include "Conversation.h"
#include "DataNode.h"
#include "Fleet.h"
#include "GameData.h"
#include "Phrase.h"
#include "Politics.h"
#include "ShipEvent.h"

#include <algorithm>

using namespace std;

namespace {
	unsigned nextID = 0;
}



// Default constructor.
Government::Government()
{
	// Default penalties:
	penaltyFor[ShipEvent::ASSIST] = -0.1;
	penaltyFor[ShipEvent::DISABLE] = 0.5;
	penaltyFor[ShipEvent::BOARD] = 0.3;
	penaltyFor[ShipEvent::CAPTURE] = 1.;
	penaltyFor[ShipEvent::DESTROY] = 1.;
	penaltyFor[ShipEvent::ATROCITY] = 10.;
	
	id = nextID++;
}



// Load a government's definition from a file.
void Government::Load(const DataNode &node)
{
	if(node.Size() >= 2)
	{
		name = node.Token(1);
		if(displayName.empty())
			displayName = name;
	}
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "display name" && child.Size() >= 2)
			displayName = child.Token(1);
		else if(child.Token(0) == "swizzle" && child.Size() >= 2)
			swizzle = child.Value(1);
		else if(child.Token(0) == "color" && child.Size() >= 4)
			color = Color(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "player reputation" && child.Size() >= 2)
			initialPlayerReputation = child.Value(1);
		else if(child.Token(0) == "crew attack" && child.Size() >= 2)
			crewAttack = max(0., child.Value(1));
		else if(child.Token(0) == "crew defense" && child.Size() >= 2)
			crewDefense = max(0., child.Value(1));
		else if(child.Token(0) == "attitude toward")
		{
			for(const DataNode &grand : child)
			{
				if(grand.Size() >= 2)
				{
					const Government *gov = GameData::Governments().Get(grand.Token(0));
					attitudeToward.resize(nextID, 0.);
					attitudeToward[gov->id] = grand.Value(1);
				}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(child.Token(0) == "penalty for")
		{
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
				{
					if(grand.Token(0) == "assist")
						penaltyFor[ShipEvent::ASSIST] = grand.Value(1);
					else if(grand.Token(0) == "disable")
						penaltyFor[ShipEvent::DISABLE] = grand.Value(1);
					else if(grand.Token(0) == "board")
						penaltyFor[ShipEvent::BOARD] = grand.Value(1);
					else if(grand.Token(0) == "capture")
						penaltyFor[ShipEvent::CAPTURE] = grand.Value(1);
					else if(grand.Token(0) == "destroy")
						penaltyFor[ShipEvent::DESTROY] = grand.Value(1);
					else if(grand.Token(0) == "atrocity")
						penaltyFor[ShipEvent::ATROCITY] = grand.Value(1);
					else
						grand.PrintTrace("Skipping unrecognized attribute:");
				}
		}
		else if(child.Token(0) == "bribe" && child.Size() >= 2)
			bribe = child.Value(1);
		else if(child.Token(0) == "fine" && child.Size() >= 2)
			fine = child.Value(1);
		else if(child.Token(0) == "enforces" && child.HasChildren())
			enforcementZones.emplace_back(child);
		else if(child.Token(0) == "enforces" && child.Size() == 2 && child.Token(1) == "all")
			enforcementZones.clear();
		else if(child.Token(0) == "death sentence" && child.Size() >= 2)
			deathSentence = GameData::Conversations().Get(child.Token(1));
		else if(child.Token(0) == "friendly hail" && child.Size() >= 2)
			friendlyHail = GameData::Phrases().Get(child.Token(1));
		else if(child.Token(0) == "friendly disabled hail" && child.Size() >= 2)
			friendlyDisabledHail = GameData::Phrases().Get(child.Token(1));
		else if(child.Token(0) == "hostile hail" && child.Size() >= 2)
			hostileHail = GameData::Phrases().Get(child.Token(1));
		else if(child.Token(0) == "hostile disabled hail" && child.Size() >= 2)
			hostileDisabledHail = GameData::Phrases().Get(child.Token(1));
		else if(child.Token(0) == "language" && child.Size() >= 2)
			language = child.Token(1);
		else if(child.Token(0) == "raid" && child.Size() >= 2)
			raidFleet = GameData::Fleets().Get(child.Token(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	
	// Default to the standard disabled hail messages.
	if(!friendlyDisabledHail)
		friendlyDisabledHail = GameData::Phrases().Get("friendly disabled");
	if(!hostileDisabledHail)
		hostileDisabledHail = GameData::Phrases().Get("hostile disabled");
}



// Get the display name of this government.
const string &Government::GetName() const
{
	return displayName;
}



// Set / Get the name used for this government in the data files.
void Government::SetName(const string &trueName)
{
	this->name = trueName;
}



const string &Government::GetTrueName() const
{
	return name;
}



// Get the color swizzle to use for ships of this government.
int Government::GetSwizzle() const
{
	return swizzle;
}



// Get the color to use for displaying this government on the map.
const Color &Government::GetColor() const
{
	return color;
}



// Get the government's initial disposition toward other governments or
// toward the player.
double Government::AttitudeToward(const Government *other) const
{
	if(!other)
		return 0.;
	if(other == this)
		return 1.;
	
	if(attitudeToward.size() <= other->id)
		return 0.;
	
	return attitudeToward[other->id];
}



double Government::InitialPlayerReputation() const
{
	return initialPlayerReputation;
}



// Get the amount that your reputation changes for the given offense.
double Government::PenaltyFor(int eventType) const
{
	double penalty = 0.;
	for(const auto &it : penaltyFor)
		if(eventType & it.first)
			penalty += it.second;
	return penalty;
}



// In order to successfully bribe this government you must pay them this
// fraction of your fleet's value. (Zero means they cannot be bribed.)
double Government::GetBribeFraction() const
{
	return bribe;
}



double Government::GetFineFraction() const
{
	return fine;
}



// Returns true if this government has no enforcement restrictions, or if the
// indicated system matches at least one enforcement zone.
bool Government::CanEnforce(const System *system) const
{
	for(const LocationFilter &filter : enforcementZones)
		if(filter.Matches(system))
			return true;
	return enforcementZones.empty();
}



// Returns true if this government has no enforcement restrictions, or if the
// indicated planet matches at least one enforcement zone.
bool Government::CanEnforce(const Planet *planet) const
{
	for(const LocationFilter &filter : enforcementZones)
		if(filter.Matches(planet))
			return true;
	return enforcementZones.empty();
}



const Conversation *Government::DeathSentence() const
{
	return deathSentence;
}



// Get a hail message (which depends on whether this is an enemy government
// and if the ship is disabled).
string Government::GetHail(bool isDisabled) const
{
	const Phrase *phrase = nullptr;
	
	if(IsEnemy())
		phrase = isDisabled ? hostileDisabledHail : hostileHail;
	else
		phrase = isDisabled ? friendlyDisabledHail : friendlyHail;
		
	return phrase ? phrase->Get() : "";
}



// Find out if this government speaks a different language.
const string &Government::Language() const
{
	return language;
}



// Pirate raids in this government's systems use this fleet definition. If
// it is null, there are no pirate raids.
const Fleet *Government::RaidFleet() const
{
	return raidFleet;
}


	
// Check if, according to the politics stored by GameData, this government is
// an enemy of the given government right now.
bool Government::IsEnemy(const Government *other) const
{
	return GameData::GetPolitics().IsEnemy(this, other);
}



// Check if this government is an enemy of the player.
bool Government::IsEnemy() const
{
	return GameData::GetPolitics().IsEnemy(this, GameData::PlayerGovernment());
}



// Check if this is the player government.
bool Government::IsPlayer() const
{
	return (this == GameData::PlayerGovernment());
}



// Commit the given "offense" against this government (which may not
// actually consider it to be an offense). This may result in temporary
// hostilities (if the even type is PROVOKE), or a permanent change to your
// reputation.
void Government::Offend(int eventType, int count) const
{
	return GameData::GetPolitics().Offend(this, eventType, count);
}



// Bribe this government to be friendly to you for one day.
void Government::Bribe() const
{
	GameData::GetPolitics().Bribe(this);
}



// Check to see if the player has done anything they should be fined for.
// Each government can only fine you once per day.
string Government::Fine(PlayerInfo &player, int scan, const Ship *target, double security) const
{
	return GameData::GetPolitics().Fine(player, this, scan, target, security);
}



// Get or set the player's reputation with this government.
double Government::Reputation() const
{
	return GameData::GetPolitics().Reputation(this);
}



void Government::AddReputation(double value) const
{
	GameData::GetPolitics().AddReputation(this, value);
}



void Government::SetReputation(double value) const
{
	GameData::GetPolitics().SetReputation(this, value);
}



double Government::CrewAttack() const
{
	return crewAttack;
}



double Government::CrewDefense() const
{
	return crewDefense;
}
