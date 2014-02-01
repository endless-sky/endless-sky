/* SpriteSet.h
Michael Zahniser, 24 Oct 2013

Class for storing sprites.
*/

#ifndef SPRITE_SET_H_INCLUDED
#define SPRITE_SET_H_INCLUDED

#include <string>

class Sprite;



class SpriteSet {
public:
	static const Sprite *Get(const std::string &name);
	
	
private:
	// Only SpriteQueue is allowed to modify the sprites.
	friend class SpriteQueue;
	static Sprite *Modify(const std::string &name);
};



#endif
