/* Sprite.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SPRITE_H_
#define SPRITE_H_

#include "Mask.h"
#include "Point.h"

#include <cstdint>
#include <vector>

class SDL_Surface;



// Class representing a drawable sprite.
class Sprite {
public:
	Sprite();
	
	void AddFrame(int frame, SDL_Surface *surface, Mask *mask = nullptr);
	
	float Width() const;
	float Height() const;
	int Frames() const;
	
	// Get the offset of the center from the top left corner; this is for easy
	// shifting of corner to center coordinates.
	Point Center() const;
	
	uint32_t Texture(int frame = 0) const;
	const Mask &GetMask(int frame = 0) const;
	
	
private:
	std::vector<uint32_t> textures;
	std::vector<Mask> masks;
	
	float width;
	float height;
};



#endif
