/* Screen.h
Michael Zahniser, 16 Oct 2013

Class that simply holds the screen dimensions. This is really just a wrapper
around some global variables.

TODO: make this thread-safe. For now I'm not bothering to because generally
OpenGL rendering will only be in a single thread, but other workers may want to
know how big a screen the objects will be in.
*/

#ifndef SCREEN_H_INCLUDED
#define SCREEN_H_INCLUDED



class Screen {
public:
	static void Set(int width, int height);
	
	static int Width();
	static int Height();
	
	
private:
	static int width;
	static int height;
};



#endif
