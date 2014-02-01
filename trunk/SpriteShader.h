/* SpriteShader.h
Michael Zahniser, 17 Oct 2013

Class for drawing sprites.
*/

#ifndef SPRITE_SHADER_H_INCLUDED
#define SPRITE_SHADER_H_INCLUDED

class Sprite;
class Point;

#include <cstdint>



class SpriteShader {
public:
	// Initialize the shaders.
	static void Init();
	
	// Draw a sprite.
	static void Draw(const Sprite *sprite, const Point &position, float zoom = 1.);
	
	static void Bind();
	static void Add(uint32_t tex0, uint32_t tex1, const float position[2], const float transform[4], int swizzle = 0, float clip = 1., float fade = 0.);
	static void Unbind();
};



#endif
