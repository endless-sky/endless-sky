/* SpriteSet.h
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

#ifndef SPRITE_SET_H_
#define SPRITE_SET_H_

#include "Sprite.h"

#include <map>
#include <mutex>
#include <set>
#include <string>



// Class for storing sprites, and for getting the sprite associated with a given
// name. If a sprite has not been loaded yet, this will still return an object
// but with no OpenGL textures associated with it (so it will draw nothing).
class SpriteSet {
public:
	// Get a pointer to the sprite data with the given name.
	const Sprite *Get(const std::string &name) const;
	Sprite *Modify(const std::string &name);

	// Inspect the sprite map and warn if some images contain no data.
	void CheckReferences() const;


private:
	mutable std::map<std::string, Sprite> sprites;
	mutable std::mutex modifyMutex;
};



#endif
