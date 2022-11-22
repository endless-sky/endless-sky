/* Government.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Government.h"

#include "Conversation.h"
#include "DataNode.h"
#include "Fleet.h"
#include "GameData.h"
#include "Outfit.h"
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
	penaltyFor[ShipEvent::SCAN_OUTFITS] = 0.;
	penaltyFor[ShipEvent::SCAN_CARGO] = 0.;
	penaltyFor[ShipEvent::PROVOKE] = 0.;
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
		bool remove = child.Token(0) == "remove";
		bool add = child.Token(0) == "add";
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}

		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = child.Size() > valueIndex;

		if(remove)
		{
			if(key == "provoked on scan")
				provokedOnScan = false;
			else if(key == "raid")
				raidFleet = nullptr;
			else if(key == "display name")
				displayName = name;
			else if(key == "death sentence")
				deathSentence = nullptr;
			else if(key == "friendly hail")
				friendlyHail = nullptr;
			else if(key == "friendly disabled hail")
				friendlyDisabledHail = nullptr;
			else if(key == "hostile hail")
				hostileHail = nullptr;
			else if(key == "hostile disabled hail")
				hostileDisabledHail = nullptr;
			else if(key == "language")
				language.clear();
			else if(key == "enforces")
				enforcementZones.clear();
			else if(key == "use foreign penalties for")
				useForeignPenaltiesFor.clear();
			else if(key == "illegals")
				illegals.clear();
			else if(key == "atrocities")
				atrocities.clear();
			else
				child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
		}
		else if(key == "attitude toward")
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
		else if(key == "penalty for")
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
					else if(grand.Token(0) == "scan")
					{
						penaltyFor[ShipEvent::SCAN_OUTFITS] = grand.Value(1);
						penaltyFor[ShipEvent::SCAN_CARGO] = grand.Value(1);
					}
					else if(grand.Token(0) == "provoke")
						penaltyFor[ShipEvent::PROVOKE] = grand.Value(1);
					else if(grand.Token(0) == "atrocity")
						penaltyFor[ShipEvent::ATROCITY] = grand.Value(1);
					else
						grand.PrintTrace("Skipping unrecognized attribute:");
				}
		}
		else if(key == "illegals")
		{
			if(!add)
				illegals.clear();
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
				{
					if(grand.Token(0) == "ignore")
						illegals[GameData::Outfits().Get(grand.Token(1))] = 0;
					else
						illegals[GameData::Outfits().Get(grand.Token(0))] = grand.Value(1);
				}
				else if(grand.Size() >= 3 && grand.Token(0) == "remove")
				{
					if(!illegals.erase(GameData::Outfits().Get(grand.Token(1))))
						grand.PrintTrace("Invalid remove, outfit not found in existing illegals:");
				}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(key == "atrocities")
		{
			if(!add)
				atrocities.clear();
			for(const DataNode &grand : child)
				if(grand.Size() >= 2)
				{
					if(grand.Token(0) == "remove" && !atrocities.erase(GameData::Outfits().Get(grand.Token(1))))
						grand.PrintTrace("Invalid remove, outfit not found in existing atrocities:");
					else if(grand.Token(0) == "ignore")
						atrocities[GameData::Outfits().Get(grand.Token(1))] = false;
				}
				else
					atrocities[GameData::Outfits().Get(grand.Token(0))] = true;
		}
		else if(key == "enforces" && child.HasChildren())
			enforcementZones.emplace_back(child);
		else if(key == "provoked on scan")
			provokedOnScan = true;
		else if(key == "use foreign penalties for")
			for(const DataNode &grand : child)
				useForeignPenaltiesFor.insert(GameData::Governments().Get(grand.Token(0))->id);
		else if(!hasValue)
			child.PrintTrace("Error: Expected key to have a value:");
		else if(key == "player reputation")
			initialPlayerReputation = add ? initialPlayerReputation + child.Value(valueIndex) : child.Value(valueIndex);
		else if(key == "crew attack")
			crewAttack = max(0., add ? child.Value(valueIndex) + crewAttack : child.Value(valueIndex));
		else if(key == "crew defense")
			crewDefense = max(0., add ? child.Value(valueIndex) + crewDefense : child.Value(valueIndex));
		else if(key == "bribe")
			bribe = add ? bribe + child.Value(valueIndex) : child.Value(valueIndex);
		else if(key == "fine")
			fine = add ? fine + child.Value(valueIndex) : child.Value(valueIndex);
		else if(add)
			child.PrintTrace("Error: Unsupported use of add:");
		else if(key == "display name")
			displayName = child.Token(valueIndex);
		else if(key == "swizzle")
			swizzle = child.Value(valueIndex);
		else if(key == "color" && child.Size() >= 3 + valueIndex)
			color = Color(child.Value(valueIndex), child.Value(valueIndex + 1), child.Value(valueIndex + 2));
		else if(key == "death sentence")
			deathSentence = GameData::Conversations().Get(child.Token(valueIndex));
		else if(key == "friendly hail")
			friendlyHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "friendly disabled hail")
			friendlyDisabledHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "hostile hail")
			hostileHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "hostile disabled hail")
			hostileDisabledHail = GameData::Phrases().Get(child.Token(valueIndex));
		else if(key == "language")
			language = child.Token(valueIndex);
		else if(key == "raid")
			raidFleet = GameData::Fleets().Get(child.Token(valueIndex));
		else if(key == "enforces" && child.Token(valueIndex) == "all")
		{
			enforcementZones.clear();
			child.PrintTrace("Warning: Deprecated use of \"enforces all\". Use \"remove enforces\" instead:");
		}
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



bool Government::Condemns(const Outfit *outfit) const
{
	const auto isAtrocity = atrocities.find(outfit);
	bool found = isAtrocity != atrocities.cend();
	return (found && isAtrocity->second) || (!found && outfit->Get("atrocity") > 0.);
}



int Government::Fines(const Outfit *outfit) const
{
	// If this government doesn't fine anything it won't fine this outfit.
	if(!fine)
		return 0;

	for(const auto &it : illegals)
		if(it.first == outfit)
			return it.second;
	return outfit->Get("illegal");
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



bool Government::IsProvokedOnScan() const
{
	return provokedOnScan;
}



bool Government::IsUsingForeignPenaltiesFor(const Government *government) const
{
	return useForeignPenaltiesFor.count(government->id);
}
