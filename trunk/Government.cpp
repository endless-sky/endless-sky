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

#include <algorithm>

using namespace std;



// Default constructor.
Government::Government()
	: name("Uninhabited"), swizzle(0), color(1.)
{
}



// Load a government's definition from a file.
void Government::Load(const DataFile::Node &node, const Set<Government> &others)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "swizzle" && child.Size() >= 2)
			swizzle = child.Value(1);
		else if(child.Token(0) == "color" && child.Size() >= 4)
			color = Color(child.Value(1), child.Value(2), child.Value(3));
		else if(child.Token(0) == "ally" && child.Size() >= 2)
			allies.insert(others.Get(child.Token(1)));
		else if(child.Token(0) == "enemy" && child.Size() >= 2)
			enemies.insert(others.Get(child.Token(1)));
	}
}



// Get the name of this government.
const string &Government::GetName() const
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



// Check whether ships of this government will come to the aid of ships of
// the given government that are under attack.
bool Government::IsAlly(const Government *other) const
{
	return (allies.find(other) != allies.end());
}



// Check whether ships of this government will preemptively attack ships of
// the given government.
bool Government::IsEnemy(const Government *other) const
{
	if(!other)
		return false;
	
	return (enemies.find(other) != enemies.end()
		|| IsProvoked(other)
		|| other->enemies.find(this) != other->enemies.end());
}



// Mark that this government is, for the moment, fighting the given
// government, which is not necessarily one of its normal enemies, because
// a ship of that government attacked it or one of its allies.
void Government::Provoke(const Government *other, double damage) const
{
	if(other != this)
		provoked[other] += damage;
}



// Check if we are provoked against the given government.
bool Government::IsProvoked(const Government *other) const
{
	return provoked[other] > 1.;
}



// Reset the record of who has provoked whom. Typically this will happen
// whenever you move to a new system.
void Government::ResetProvocation() const
{
	provoked.clear();
}



// Every time step, the provokation values fade a little:
void Government::CoolDown() const
{
	for(auto &it : provoked)
		it.second = max(0., it.second - 1.);
}
