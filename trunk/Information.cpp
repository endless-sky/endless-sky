/* Information.cpp
Michael Zahniser, 24 Oct 2013

Function definitions for the Information class.
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
