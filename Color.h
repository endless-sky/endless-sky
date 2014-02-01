/* Color.h
Michael Zahniser, 5 Nov 2013

Class representing an RGBA color (for use by OpenGL).
*/

#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED



class Color {
public:
	Color(float i = 1.f, float a = 1.f);
	Color(float r, float g, float b, float a = 1.f);
	
	void Load(double r, double g, double b, double a);
	const float *Get() const;
	
	
private:
	float color[4];
};



#endif
