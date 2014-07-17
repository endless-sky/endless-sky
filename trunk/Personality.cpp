/* Personality.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Personality.h"

#include "Angle.h"
#include "DataNode.h"

using namespace std;

namespace {
	static const int PACIFIST = 1;
	static const int FORBEARING = 2;
	static const int TIMID = 4;
	static const int DISABLES = 8;
	static const int PLUNDERS = 16;
	static const int HEROIC = 32;
}



// Default settings for player's ships.
Personality::Personality()
	: flags(DISABLES), confusionMultiplier(10. * .001)
{
}



void Personality::Load(const DataNode &node)
{
	flags = 0;
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "confusion" && child.Size() >= 2)
			confusionMultiplier = child.Value(1) * .001;
		else if(child.Token(0) == "pacifist")
			flags |= PACIFIST;
		else if(child.Token(0) == "forbearing")
			flags |= FORBEARING;
		else if(child.Token(0) == "timid")
			flags |= TIMID;
		else if(child.Token(0) == "disables")
			flags |= DISABLES;
		else if(child.Token(0) == "plunders")
			flags |= PLUNDERS;
		else if(child.Token(0) == "heroic")
			flags |= HEROIC;
	}
}



bool Personality::IsPacifist() const
{
	return flags & PACIFIST;
}



bool Personality::IsForbearing() const
{
	return flags & FORBEARING;
}



bool Personality::IsTimid() const
{
	return flags & TIMID;
}



bool Personality::Disables() const
{
	return flags & DISABLES;
}



bool Personality::Plunders() const
{
	return flags & PLUNDERS;
}



bool Personality::IsHeroic() const
{
	return flags & HEROIC;
}



const Point &Personality::Confusion() const
{
	confusion += Angle::Random().Unit() * confusionMultiplier;
	confusion *= .999;
	return confusion;
}
