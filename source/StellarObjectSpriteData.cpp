/* StellarObjectSpriteData.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "StellarObjectSpriteData.h"

#include "DataNode.h"
#include "image/SpriteSet.h"

using namespace std;



StellarObjectSpriteData::StellarObjectSpriteData(const DataNode &node)
{
	Load(node);
}



void StellarObjectSpriteData::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "landing message" && hasValue)
			landingMessage = child.Value(1);
		else if(key == "power" && hasValue)
			solarPower = max(0., child.Value(1));
		else if(key == "wind" && hasValue)
			solarWind = max(0., child.Value(1));
		else if(key == "icon" && hasValue)
			starIcon = SpriteSet::Get(child.Token(1));
		else if(key == "habitable" && hasValue)
			habitable = child.Value(1);
		else if(key == "mass" && hasValue)
		{
			mass = max(0., child.Value(1));
			if(!mass)
				child.PrintTrace("A star or stellar object's mass must be greater than 0.");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void StellarObjectSpriteData::SetLandingMessage(const std::string &message)
{
	landingMessage = message;
}



void StellarObjectSpriteData::SetMass(double mass)
{
	this->mass = mass;
}



const std::string &StellarObjectSpriteData::LandingMessage() const
{
	return landingMessage;
}



double StellarObjectSpriteData::SolarPower() const
{
	return solarPower;
}



double StellarObjectSpriteData::SolarWind() const
{
	return solarWind;
}



const Sprite *StellarObjectSpriteData::StarIcon() const
{
	return starIcon;
}



double StellarObjectSpriteData::HabitableDistance() const
{
	return habitable;
}



double StellarObjectSpriteData::Mass() const
{
	return mass;
}
