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

#include "Outfit.h"
#include "Ship.h"
#include "SpriteSet.h"

using namespace std;



// Default constructor.
Planet::Planet()
	: landscape(nullptr)
{
}



// Load a planet's description from a file.
void Planet::Load(const DataFile::Node &node, const Set<Sale<Ship>> &ships, const Set<Sale<Outfit>> &outfits)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "landscape" && child.Size() >= 2)
			landscape = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			description += child.Token(1);
			description += '\n';
		}
		else if(child.Token(0) == "spaceport" && child.Size() >= 2)
		{
			spaceport += child.Token(1);
			spaceport += '\n';
		}
		else if(child.Token(0) == "shipyard" && child.Size() >= 2)
			shipSales.push_back(ships.Get(child.Token(1)));
		else if(child.Token(0) == "outfitter" && child.Size() >= 2)
			outfitSales.push_back(outfits.Get(child.Token(1)));
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


	
// Check if this planet has a shipyard.
bool Planet::HasShipyard() const
{
	return !Shipyard().empty();
}



// Get the list of ships in the shipyard.
const Sale<Ship> &Planet::Shipyard() const
{
	if(shipyard.empty() && !shipSales.empty())
	{
		for(const Sale<Ship> *sale : shipSales)
			shipyard.Add(*sale);
	}
	
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
	if(outfitter.empty() && !outfitSales.empty())
	{
		for(const Sale<Outfit> *sale : outfitSales)
			outfitter.Add(*sale);
	}
	
	return outfitter;
}
