/* OutlineShader.h
Michael Zahniser, 22 Oct 2013

*/

#ifndef OUTLINE_SHADER_H_INCLUDED
#define OUTLINE_SHADER_H_INCLUDED

class Sprite;
class Point;



class OutlineShader {
public:
	static void Init();
	
	static void Draw(const Sprite *sprite, const Point &pos, const Point &size);
};



#endif
