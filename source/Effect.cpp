/* Effect.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Effect.h"

#include "audio/Audio.h"
#include "DataNode.h"

#include <map>

using namespace std;

namespace {
	const map<string, SoundCategory> categoryNames = {
		{"ui", SoundCategory::UI},
		{"anti-missile", SoundCategory::ANTI_MISSILE},
		{"weapon", SoundCategory::WEAPON},
		{"engine", SoundCategory::ENGINE},
		{"afterburner", SoundCategory::AFTERBURNER},
		{"jump", SoundCategory::JUMP},
		{"explosion", SoundCategory::EXPLOSION},
		{"scan", SoundCategory::SCAN},
		{"environment", SoundCategory::ENVIRONMENT},
		{"alert", SoundCategory::ALERT}
	};
}



const string &Effect::TrueName() const
{
	return trueName;
}



void Effect::SetTrueName(const string &name)
{
	this->trueName = name;
}



void Effect::Load(const DataNode &node)
{
	if(node.Size() > 1)
		trueName = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "sprite")
			LoadSprite(child);
		else if(key == "zooms")
			inheritsZoom = true;
		else if(key == "sound" && hasValue)
			sound = Audio::Get(child.Token(1));
		else if(key == "sound category" && hasValue)
		{
			if(categoryNames.contains(child.Token(1)))
				soundCategory = categoryNames.at(child.Token(1));
			else
				child.PrintTrace("Unknown sound category \"" + child.Token(1) + "\"");
		}
		else if(key == "lifetime" && hasValue)
			lifetime = child.Value(1);
		else if(key == "random lifetime" && hasValue)
			randomLifetime = child.Value(1);
		else if(key == "velocity scale" && hasValue)
			velocityScale = child.Value(1);
		else if(key == "random velocity" && hasValue)
			randomVelocity = child.Value(1);
		else if(key == "random angle" && hasValue)
			randomAngle = child.Value(1);
		else if(key == "random spin" && hasValue)
			randomSpin = child.Value(1);
		else if(key == "random frame rate" && hasValue)
			randomFrameRate = child.Value(1);
		else if(key == "absolute angle" && hasValue)
		{
			absoluteAngle = Angle(child.Value(1));
			hasAbsoluteAngle = true;
		}
		else if(key == "absolute velocity" && hasValue)
		{
			absoluteVelocity = child.Value(1);
			hasAbsoluteVelocity = true;
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}
