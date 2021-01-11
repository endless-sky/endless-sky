/* SpriteSet.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SPRITE_SET_H_
#define SPRITE_SET_H_

#include <set>
#include <string>

class Sprite;



// Class for storing sprites, and for getting the sprite associated with a given
// name. If a sprite has not been loaded yet, this will still return an object
// but with no OpenGL textures associated with it (so it will draw nothing).
class SpriteSet {
public:
	// Get a pointer to the sprite data with the given name.
	static const Sprite *Get(const std::string &name);
	
	// Inspect the sprite map and return any paths that loaded no data.
	static std::set<std::string> CheckReferences();
	
	
private:
	// Only SpriteQueue is allowed to modify the sprites.
	friend class SpriteQueue;
	static Sprite *Modify(const std::string &name);
};



#endif
