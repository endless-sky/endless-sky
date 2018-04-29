/* Planet.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Planet.h"

#include "DataNode.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "PlayerInfo.h"
#include "Politics.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "SpriteSet.h"
#include "System.h"

using namespace std;

namespace {
	const string WORMHOLE = "wormhole";
	const string PLANET = "planet";
}



// Load a planet's description from a file.
void Planet::Load(const DataNode &node, const Set<Sale<Ship>> &ships, const Set<Sale<Outfit>> &outfits)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	
	// If this planet has been loaded before, these sets of items should be
	// reset instead of appending to them:
	set<string> shouldOverwrite = {"attributes", "description", "spaceport"};
	
	for(const DataNode &child : node)
	{
		// Check for the "add" or "remove" keyword.
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}
		
		// Get the key and value (if any).
		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = (child.Size() > valueIndex);
		const string &value = child.Token(hasValue ? valueIndex : 0);
		
		// Check for conditions that require clearing this key's current value.
		// "remove <key>" means to clear the key's previous contents.
		// "remove <key> <value>" means to remove just that value from the key.
		bool removeAll = (remove && !hasValue);
		// "<key> clear" is the deprecated way of writing "remove <key>."
		removeAll |= (!add && !remove && hasValue && value == "clear");
		// If this is the first entry for the given key, and we are not in "add"
		// or "remove" mode, its previous value should be cleared.
		bool overwriteAll = (!add && !remove && !removeAll && shouldOverwrite.count(key));
		// Clear the data of the given type.
		if(removeAll || overwriteAll)
		{
			// Clear the data of the given type.
			if(key == "music")
				music.clear();
			else if(key == "attributes")
				attributes.clear();
			else if(key == "description")
				description.clear();
			else if(key == "spaceport")
				spaceport.clear();
			else if(key == "shipyard")
				shipSales.clear();
			else if(key == "outfitter")
				outfitSales.clear();
			else if(key == "government")
				government = nullptr;
			else if(key == "required reputation")
				requiredReputation = 0.;
			else if(key == "bribe")
				bribe = 0.;
			else if(key == "security")
				security = 0.;
			else if(key == "tribute")
				tribute = 0;
			
			// If not in "overwrite" mode, move on to the next node.
			if(overwriteAll)
				shouldOverwrite.erase(key);
			else
				continue;
		}
		
		// Handle the attributes which can be "removed."
		if(!hasValue)
		{
			child.PrintTrace("Expected key to have a value:");
			continue;
		}
		else if(key == "attributes")
		{
			if(remove)
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.erase(child.Token(i));
			else
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.insert(child.Token(i));
		}
		else if(key == "shipyard")
		{
			if(remove)
				shipSales.erase(ships.Get(value));
			else
				shipSales.insert(ships.Get(value));
		}
		else if(key == "outfitter")
		{
			if(remove)
				outfitSales.erase(outfits.Get(value));
			else
				outfitSales.insert(outfits.Get(value));
		}
		// Handle the attributes which cannot be "removed."
		else if(remove)
		{
			child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
			continue;
		}
		else if(key == "landscape")
			landscape = SpriteSet::Get(value);
		else if(key == "music")
			music = value;
		else if(key == "description" || key == "spaceport")
		{
			string &text = (key == "description") ? description : spaceport;
			if(!text.empty() && !value.empty() && value[0] > ' ')
				text += '\t';
			text += value;
			text += '\n';
		}
		else if(key == "government")
			government = GameData::Governments().Get(value);
		else if(key == "required reputation")
			requiredReputation = child.Value(valueIndex);
		else if(key == "bribe")
			bribe = child.Value(valueIndex);
		else if(key == "security")
			security = child.Value(valueIndex);
		else if(key == "tribute")
		{
			tribute = child.Value(valueIndex);
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "threshold" && grand.Size() >= 2)
					defenseThreshold = grand.Value(1);
				else if(grand.Token(0) == "fleet" && grand.Size() >= 3)
				{
					defenseCount = (grand.Size() >= 3 ? grand.Value(2) : 1);
					defenseFleet = GameData::Fleets().Get(grand.Token(1));
				}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	
	static const vector<string> AUTO_ATTRIBUTES = {"spaceport", "shipyard", "outfitter"};
	bool autoValues[3] = {!spaceport.empty(), !shipSales.empty(), !outfitSales.empty()};
	for(unsigned i = 0; i < AUTO_ATTRIBUTES.size(); ++i)
	{
		if(autoValues[i])
			attributes.insert(AUTO_ATTRIBUTES[i]);
		else
			attributes.erase(AUTO_ATTRIBUTES[i]);
	}

	inhabited = (HasSpaceport() || requiredReputation || defenseFleet) && !attributes.count("uninhabited");
}



// Get the name of the planet.
const string &Planet::Name() const
{
	static const string UNKNOWN = "???";
	if(IsWormhole())
		return UNKNOWN;
	return name;
}



// Get the name used for this planet in the data files.
const string &Planet::TrueName() const
{
	return name;
}



// Get the planet's descriptive text.
const string &Planet::Description() const
{
	return description;
}



// Get the landscape sprite.
const Sprite *Planet::Landscape() const
{
	return landscape;
}



// Get the name of the ambient audio to play on this planet.
const string &Planet::MusicName() const
{
	return music;
}



// Get the list of "attributes" of the planet.
const set<string> &Planet::Attributes() const
{
	return attributes;
}



// Get planet's noun descriptor from attributes
const string &Planet::Noun() const
{
	if(IsWormhole())
		return WORMHOLE;
	
	for(const string &attribute : attributes)
		if(attribute == "moon" || attribute == "station")
			return attribute;
	
	return PLANET;
}



// Check whether there is a spaceport (which implies there is also trading,
// jobs, banking, and hiring).
bool Planet::HasSpaceport() const
{
	return !spaceport.empty();
}



// Get the spaceport's descriptive text.
const string &Planet::SpaceportDescription() const
{
	return spaceport;
}



// Check if this planet is inhabited (i.e. it has a spaceport, and does not
// have the "uninhabited" attribute).
bool Planet::IsInhabited() const
{
	return inhabited;
}


	
// Check if this planet has a shipyard.
bool Planet::HasShipyard() const
{
	return !Shipyard().empty();
}



// Get the list of ships in the shipyard.
const Sale<Ship> &Planet::Shipyard() const
{
	shipyard.clear();
	for(const Sale<Ship> *sale : shipSales)
		shipyard.Add(*sale);
	
	return shipyard;
}



// Check if this planet has an outfitter.
bool Planet::HasOutfitter() const
{
	return !Outfitter().empty();
}



// Get the list of outfits available from the outfitter.
const Sale<Outfit> &Planet::Outfitter() const
{
	outfitter.clear();
	for(const Sale<Outfit> *sale : outfitSales)
		outfitter.Add(*sale);
	
	return outfitter;
}



// Get this planet's government. Most planets follow the government of the system they are in.
const Government *Planet::GetGovernment() const
{
	return government ? government : systems.empty() ? nullptr : GetSystem()->GetGovernment();
}



// You need this good a reputation with this system's government to land here.
double Planet::RequiredReputation() const
{
	return requiredReputation;
}



// This is what fraction of your fleet's value you must pay as a bribe in
// order to land on this planet. (If zero, you cannot bribe it.)
double Planet::GetBribeFraction() const
{
	return bribe;
}



// This is how likely the planet's authorities are to notice if you are
// doing something illegal.
double Planet::Security() const
{
	return security;
}



const System *Planet::GetSystem() const
{
	return (systems.empty() ? nullptr : systems.front());
}



// Check if this planet is in the given system. Note that wormholes may be
// in more than one system.
bool Planet::IsInSystem(const System *system) const
{
	return (find(systems.begin(), systems.end(), system) != systems.end());
}



void Planet::SetSystem(const System *system)
{
	if(find(systems.begin(), systems.end(), system) == systems.end())
		systems.push_back(system);
}



// Remove the given system from the list of systems this planet is in. This
// must be done when game events rearrange the planets in a system.
void Planet::RemoveSystem(const System *system)
{
	auto it = find(systems.begin(), systems.end(), system);
	if(it != systems.end())
		systems.erase(it);
}



// Check if this is a wormhole (that is, it appears in multiple systems).
bool Planet::IsWormhole() const
{
	return (systems.size() > 1);
}



const System *Planet::WormholeSource(const System *to) const
{
	auto it = find(systems.begin(), systems.end(), to);
	if(it == systems.end())
		return to;
	
	return (it == systems.begin() ? systems.back() : *--it);
}




const System *Planet::WormholeDestination(const System *from) const
{
	auto it = find(systems.begin(), systems.end(), from);
	if(it == systems.end())
		return from;
	
	++it;
	return (it == systems.end() ? systems.front() : *it);
}



const vector<const System *> &Planet::WormholeSystems() const
{
	return systems;
}



// Check if the given ship has all the attributes necessary to allow it to
// land on this planet.
bool Planet::IsAccessible(const Ship *ship) const
{
	// Check whether any of this planet's attributes are in the form of the
	// string "requires: <attribute>"; if so the ship must have that attribute.
	static const string PREFIX = "requires: ";
	static const string PREFIX_END = "requires:!";
	auto it = attributes.lower_bound(PREFIX);
	auto end = attributes.lower_bound(PREFIX_END);
	if(it == end)
		return true;
	if(!ship)
		return false;
	
	for( ; it != end; ++it)
		if(!ship->Attributes().Get(it->substr(PREFIX.length())))
			return false;
	
	return true;
}



// Below are convenience functions which access the game state in Politics,
// but do so with a less convoluted syntax:
bool Planet::HasFuelFor(const Ship &ship) const
{
	return !IsWormhole() && HasSpaceport() && CanLand(ship);
}



bool Planet::CanLand(const Ship &ship) const
{
	return IsAccessible(&ship) && GameData::GetPolitics().CanLand(ship, this);
}



bool Planet::CanLand() const
{
	return GameData::GetPolitics().CanLand(this);
}



bool Planet::CanUseServices() const
{
	return GameData::GetPolitics().CanUseServices(this);
}



void Planet::Bribe(bool fullAccess) const
{
	GameData::GetPolitics().BribePlanet(this, fullAccess);
}



// Demand tribute, and get the planet's response.
string Planet::DemandTribute(PlayerInfo &player) const
{
	if(player.GetCondition("tribute: " + name))
		return "We are already paying you as much as we can afford.";
	if(!tribute || !defenseFleet || !defenseCount || player.GetCondition("combat rating") < defenseThreshold)
		return "Please don't joke about that sort of thing.";
	
	// The player is scary enough for this planet to take notice. Check whether
	// this is the first demand for tribute, or not.
	if(!isDefending)
	{
		isDefending = true;
		GameData::GetPolitics().Offend(defenseFleet->GetGovernment(), ShipEvent::PROVOKE);
		GameData::GetPolitics().Offend(GetGovernment(), ShipEvent::ATROCITY);
		return "Our defense fleet will make short work of you.";
	}
	
	// The player has already demanded tribute. Have they killed off the entire
	// defense fleet?
	bool isDefeated = (defenseDeployed == defenseCount);
	for(const shared_ptr<Ship> &ship : defenders)
		if(!ship->IsDisabled() && !ship->IsYours())
		{
			isDefeated = false;
			break;
		}
	
	if(!isDefeated)
		return "We're not ready to surrender yet.";
	
	player.Conditions()["tribute: " + name] = tribute;
	GameData::GetPolitics().DominatePlanet(this);
	return "We surrender. We will pay you " + Format::Number(tribute) + " credits per day to leave us alone.";
}



void Planet::DeployDefense(list<shared_ptr<Ship>> &ships) const
{
	if(!isDefending || Random::Int(60) || defenseDeployed == defenseCount)
		return;
	
	// Have another defense fleet take off from the planet.
	auto end = defenders.begin();
	defenseFleet->Enter(*GetSystem(), defenders, this);
	ships.insert(ships.begin(), defenders.begin(), end);
	
	// All defenders get a special personality.
	Personality defenderPersonality = Personality::Defender();
	for(auto it = defenders.begin(); it != end; ++it)
		(**it).SetPersonality(defenderPersonality);
	
	++defenseDeployed;
}



void Planet::ResetDefense() const
{
	isDefending = false;
	defenseDeployed = 0;
	defenders.clear();
}
