/* Galaxy.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Galaxy.h"

#include "DataNode.h"
#include "Files.h"
#include "GameData.h"
#include "SpriteSet.h"
#include "System.h"

using namespace std;



void Galaxy::Load(const DataNode &node, Set<Galaxy> &galaxies)
{
	name = node.Token(1);
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "pos" && child.Size() >= 3)
			position = Point(child.Value(1), child.Value(2));
		else if(child.Token(0) == "sprite" && child.Size() >= 2)
			LoadSprite(child);
		else if(child.Token(0) == "label" && child.Size() >= 2)
		{
			auto *label = galaxies.Get("label " + child.Token(1));
			label->Load(child, galaxies);
			label->name = "label " + child.Token(1);
			AddLabel(label);
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const string &Galaxy::Name() const
{
	return name;
}



const Point &Galaxy::Position() const
{
	return position;
}



const set<const System *> &Galaxy::Systems() const
{
	return systems;
}



const vector<const Galaxy *> &Galaxy::Labels() const
{
	return labels;
}



void Galaxy::SetName(const string &name)
{
	this->name = name;
}



void Galaxy::SetPosition(const Point &pos)
{
	position = pos;
}



void Galaxy::AddLabel(const Galaxy *label)
{
	labels.emplace_back(label);
	if(!label->Labels().empty())
		Files::LogError("Warning: galaxy label with labels themselves are ignored.");
}



void Galaxy::ClearLabels()
{
	labels.clear();
}



void Galaxy::AddSystem(const System *system)
{
	systems.insert(system);
}



void Galaxy::RemoveSystem(const System *system)
{
	systems.erase(system);
}
