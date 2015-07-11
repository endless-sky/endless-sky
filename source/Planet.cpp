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
#include "GameData.h"
#include "Politics.h"
#include "Ship.h"
#include "SpriteSet.h"

using namespace std;



// Load a planet's description from a file.
void Planet::Load(const DataNode &node, const Set<Sale<Ship>> &ships, const Set<Sale<Outfit>> &outfits)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	
	// If this planet has been loaded before, these sets of items should be
	// reset if they are also defined here, instead of appending to them:
	bool resetAttributes = !attributes.empty();
	bool resetDescription = !description.empty();
	bool resetSpaceport = !spaceport.empty();
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "landscape" && child.Size() >= 2)
			landscape = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "attributes")
		{
			if(resetAttributes)
			{
				resetAttributes = false;
				attributes.clear();
			}
			for(int i = 1; i < child.Size(); ++i)
				attributes.insert(child.Token(i));
		}
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			if(resetDescription)
			{
				resetDescription = false;
				description.clear();
			}
			if(!description.empty() && !child.Token(1).empty() && child.Token(1)[0] > ' ')
				description += '\t';
			description += child.Token(1);
			description += '\n';
		}
		else if(child.Token(0) == "spaceport" && child.Size() >= 2)
		{
			if(resetSpaceport)
			{
				resetSpaceport = false;
				spaceport.clear();
			}
			if(!spaceport.empty() && !child.Token(1).empty() && child.Token(1)[0] > ' ')
				spaceport += '\t';
			spaceport += child.Token(1);
			spaceport += '\n';
		}
		else if(child.Token(0) == "shipyard" && child.Size() >= 2)
		{
			if(child.Token(1) == "clear")
				shipSales.clear();
			else
				shipSales.push_back(ships.Get(child.Token(1)));
		}
		else if(child.Token(0) == "outfitter" && child.Size() >= 2)
		{
			if(child.Token(1) == "clear")
				outfitSales.clear();
			else
				outfitSales.push_back(outfits.Get(child.Token(1)));
		}
		else if(child.Token(0) == "required reputation" && child.Size() >= 2)
			requiredReputation = child.Value(1);
		else if(child.Token(0) == "bribe" && child.Size() >= 2)
			bribe = child.Value(1);
		else if(child.Token(0) == "security" && child.Size() >= 2)
			security = child.Value(1);
	}
}



// Get the name of the planet.
const string &Planet::Name() const
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



// Get the list of "attributes" of the planet.
const set<string> &Planet::Attributes() const
{
	return attributes;
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
	return HasSpaceport() && attributes.find("uninhabited") == attributes.end();
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



const System *Planet::WormholeDestination(const System *from) const
{
	auto it = find(systems.begin(), systems.end(), from);
	if(it == systems.end())
		return from;
	
	++it;
	return (it == systems.end() ? systems.front() : *it);
}



// Below are convenience functions which access the game state in Politics,
// but do so with a less convoluted syntax:
bool Planet::CanLand(const Ship &ship) const
{
	return GameData::GetPolitics().CanLand(ship, this);
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
