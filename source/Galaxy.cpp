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
#include "SpriteSet.h"

using namespace std;



void Galaxy::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "pos" && child.Size() >= 3)
			position = Point(child.Value(1), child.Value(2));
		else if(child.Token(0) == "sprite" && child.Size() >= 2)
			sprite = SpriteSet::Get(child.Token(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const Point &Galaxy::Position() const
{
	return position;
}



const Sprite *Galaxy::GetSprite() const
{
	return sprite;
}
