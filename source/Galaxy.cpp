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



Galaxy::Label::Label(const DataNode &node)
{
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "pos" && child.Size() >= 3)
			position = Point(child.Value(1), child.Value(2));
		else if(child.Token(0) == "sprite" && child.Size() >= 2)
			LoadSprite(child);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



Galaxy::Label::Label(const Galaxy *galaxy)
{
	*static_cast<Body *>(this) = *galaxy;
}



void Galaxy::Load(const DataNode &node, Set<Galaxy> &galaxies)
{
	name = node.Token(1);

	bool isLabel = false;
	if(!name.compare(0, 6, "label "))
	{
		Files::LogError("Warning: root level galaxy label \"" + name + "\" is deprecated.");
		isLabel = true;
	}

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "pos" && child.Size() >= 3)
			position = Point(child.Value(1), child.Value(2));
		else if(child.Token(0) == "sprite" && child.Size() >= 2)
			LoadSprite(child);
		else if(child.Token(0) == "label" && !isLabel)
			labels.emplace_back(child);
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



const vector<Galaxy::Label> &Galaxy::Labels() const
{
	return labels;
}



void Galaxy::SetName(const string &name)
{
	this->name = name;
}



void Galaxy::SetPosition(Point pos)
{
	position = std::move(pos);
}



void Galaxy::AddLabel(Galaxy *label)
{
	if(!label->parent)
	{
		labels.emplace_back(label);
		label->parent = this;
	}
	else
	{
		// Since the label is already in the list, we need to update
		// the entry to the new galaxy value.
		auto it = find_if(labels.begin(), labels.end(), [&label](const Label &l)
				{
					return l.name == label->name;
				});
		if(it != labels.end())
			static_cast<Body &>(*it) = *label;
	}
}



void Galaxy::AddSystem(const System *system)
{
	systems.insert(system);
}



void Galaxy::RemoveSystem(const System *system)
{
	systems.erase(system);
}
