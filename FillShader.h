/* FillShader.h
Michael Zahniser, 14 Dec 2013

Class holding a function to fill a rectangular region of the screen with a given
color (which may be partly translucent).
*/

#ifndef FILL_SHADER_H_INCLUDED
#define FILL_SHADER_H_INCLUDED

class Point;



class FillShader {
public:
	static void Init();
	
	static void Fill(const Point &center, const Point &size, const float *color = nullptr);
};



#endif
