/* SpriteSet.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpriteSet.h"

#include "Sprite.h"

#include <map>

using namespace std;

namespace {
	map<string, Sprite> sprites;
}



const Sprite *SpriteSet::Get(const string &name)
{
	return Modify(name);
}



Sprite *SpriteSet::Modify(const string &name)
{
	auto it = sprites.find(name);
	if(it == sprites.end())
		it = sprites.insert(make_pair(name, Sprite(name))).first;
	return &it->second;
}
