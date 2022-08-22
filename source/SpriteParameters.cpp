/* SpriteParameters.cpp
Copyright (c) 2022 by XessWaffle

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpriteParameters.h"

#include "Sprite.h"

SpriteParameters::SpriteParameters()
{

}

SpriteParameters::SpriteParameters(const Sprite* sprite)
{
	this->sprites.insert(std::pair<std::string, const Sprite*>("default", sprite));
}

void SpriteParameters::SetSprite(std::string trigger, const Sprite* sprite)
{
	this->sprites.insert(std::pair<std::string, const Sprite*>(trigger, sprite));
}


const Sprite *SpriteParameters::GetSprite(std::string trigger) const
{
	auto it = this->sprites.find(trigger);

	if(it != this->sprites.end()){
		return it->second;
	} else {
		return nullptr;
	}
}