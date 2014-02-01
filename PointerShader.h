/* PointerShader.h
Michael Zahniser, 6 Nov 2013

*/

#ifndef POINTER_SHADER_H_INCLUDED
#define POINTER_SHADER_H_INCLUDED

#include "Point.h"



class PointerShader {
public:
	static void Init();
	
	static void Draw(const Point &center, const Point &angle, float width, float height, float offset, const float *color = nullptr);
	
	static void Bind();
	static void Add(const Point &center, const Point &angle, float width, float height, float offset, const float *color);
	static void Unbind();
};



#endif
