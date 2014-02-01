/* Sprite.h
Michael Zahniser, 17 Oct 2013

Class representing a drawable sprite.
*/

#ifndef SPRITE_H_INCLUDED
#define SPRITE_H_INCLUDED

#include "Mask.h"

#include <cstdint>
#include <vector>

class SDL_Surface;



class Sprite {
public:
	Sprite();
	
	void AddFrame(int frame, SDL_Surface *surface, Mask *mask = nullptr);
	
	float Width() const;
	float Height() const;
	int Frames() const;
	
	uint32_t Texture(int frame = 0) const;
	const Mask &GetMask(int frame = 0) const;
	
	
private:
	std::vector<uint32_t> textures;
	std::vector<Mask> masks;
	
	float width;
	float height;
};



#endif
