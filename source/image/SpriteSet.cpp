/* SpriteSet.cpp
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

#include "SpriteSet.h"

#include "../Logger.h"
#include "Sprite.h"

#include <map>
#include <mutex>

using namespace std;

namespace {
	map<string, Sprite> sprites;

	mutex modifyMutex;
}



const Sprite *SpriteSet::Get(const string &name)
{
	return Modify(name);
}



void SpriteSet::CheckReferences()
{
	for(const auto &pair : sprites)
	{
		const Sprite &sprite = pair.second;
		if(sprite.Height() == 0 && sprite.Width() == 0)
			// Landscapes are allowed to still be empty.
			if(!pair.first.starts_with("land/"))
				Logger::Log("Image \"" + pair.first + "\" is referred to, but has no pixels.", Logger::Level::WARNING);
	}
}



Sprite *SpriteSet::Modify(const string &name)
{
	lock_guard<mutex> guard(modifyMutex);

	auto it = sprites.find(name);
	if(it == sprites.end())
		it = sprites.emplace(name, Sprite(name)).first;
	return &it->second;
}
