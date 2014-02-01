/* DrawList.h
Michael Zahniser, 17 Oct 2013

Class for storing a list of textures to blit to the screen.

Swizzle is: R = 0, G = 1, B = 2, Z = 3, O = 4.
Rs + 5 * Gs + 25 * Bs, so no swizzle (RGB) = 0 + 5 + 50 = 55.
*/

#ifndef DRAW_LIST_H_INCLUDED
#define DRAW_LIST_H_INCLUDED

#include "Point.h"

class Animation;
class Sprite;

#include <cstdint>
#include <vector>



class DrawList {
public:
	// Default constructor.
	DrawList();
	
	// Clear the list, also setting the global time step for animation.
	void Clear(int step = 0);
	
	// Add an animation.
	void Add(const Animation &animation, Point pos, Point unit, double clip = 1.);
	
	// Draw all the items in this list. The shader object may be shared between
	// multiple DrawLists, so pass it in here.
	void Draw() const;
	
	
private:
	class Item {
	public:
		Item() = default;
		Item(const Animation &animation, Point pos, Point unit, float clip, int step);
		
		// Get the texture of this sprite.
		uint32_t Texture0() const;
		uint32_t Texture1() const;
		
		// These two items can be uploaded directly to the shader:
		// Get the (x, y) position of the center of the sprite.
		const float *Position() const;
		// Get the [a, b; c, d] size and rotation matrix.
		const float *Transform() const;
		
		// Get the color swizzle.
		uint32_t Swizzle() const;
		
		float Clip() const;
		float Fade() const;
		
	private:
		uint32_t tex0;
		uint32_t tex1;
		float position[2];
		float transform[4];
		float clip;
		uint32_t flags;
	};
	
	
private:
	int step;
	std::vector<Item> items;
};



#endif
