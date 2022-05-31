/* SoundSet.h
Copyright (c) 2022 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SOUND_SET_H_
#define SOUND_SET_H_

#include "Sound.h"

#include <map>
#include <mutex>
#include <set>
#include <string>



// Class for storing sprites, and for getting the sprite associated with a given
// name. If a sprite has not been loaded yet, this will still return an object
// but with no OpenGL textures associated with it (so it will draw nothing).
class SoundSet {
public:
	// Get a pointer to the sound with the given name.
	const Sound *Get(const std::string &name) const;
	Sound *Modify(const std::string &name);

	// Inspect the sound map and warn if some sounds contain no data.
	void CheckReferences() const;

	// Provide iterators for looping over all the sounds.
	std::map<std::string, Sound>::const_iterator begin() const { return sounds.begin(); };
	std::map<std::string, Sound>::const_iterator end() const { return sounds.end(); };

private:
	mutable std::map<std::string, Sound> sounds;
	mutable std::mutex modifyMutex;
};



#endif
