/* Information.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Information.h"

#include "Sprite.h"

using namespace std;



Information::Information()
	: radar(nullptr)
{
}



void Information::SetSprite(const string &name, const Sprite *sprite)
{
	sprites[name] = sprite;
}



const Sprite *Information::GetSprite(const string &name) const
{
	static const Sprite empty;
	
	auto it = sprites.find(name);
	return (it == sprites.end()) ? &empty : it->second;
}



void Information::SetString(const string &name, const string &value)
{
	strings[name] = value;
}



const string &Information::GetString(const string &name) const
{
	static const string empty;
	
	auto it = strings.find(name);
	return (it == strings.end()) ? empty : it->second;
}



void Information::SetBar(const string &name, double value, double segments)
{
	bars[name] = value;
	barSegments[name] = static_cast<double>(segments);
}



double Information::BarValue(const string &name) const
{
	auto it = bars.find(name);
	
	return (it == bars.end()) ? 1. : it->second;
}



double Information::BarSegments(const string &name) const
{
	auto it = barSegments.find(name);
	
	return (it == barSegments.end()) ? 1. : it->second;
}


	
void Information::SetRadar(const Radar &radar)
{
	this->radar = &radar;
}



const Radar *Information::GetRadar() const
{
	return radar;
}


	
void Information::SetCondition(const string &condition)
{
	conditions.insert(condition);
}



bool Information::HasCondition(const string &condition) const
{
	if(condition.empty())
		return true;
	
	return (conditions.find(condition) != conditions.end());
}
