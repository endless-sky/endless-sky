/* LineShader.h
Michael Zahniser, 23 Oct 2013

*/

#ifndef LINE_SHADER_H_INCLUDED
#define LINE_SHADER_H_INCLUDED

#include "Point.h"



class LineShader {
public:
	static void Init();
	
	static void Draw(Point from, Point to, float width, const float *color = nullptr);
};



#endif
