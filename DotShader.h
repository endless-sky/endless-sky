/* DotShader.h
Michael Zahniser, 31 Oct 2013

*/

#ifndef DOT_SHADER_H_INCLUDED
#define DOT_SHADER_H_INCLUDED

#include "Point.h"



class DotShader {
public:
	static void Init();
	
	static void Draw(const Point &pos, float out, float in, const float *color = nullptr);
	
	static void Bind();
	static void Add(const Point &pos, float out, float in, const float *color);
	static void Unbind();
};



#endif
