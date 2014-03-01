/* Screen.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SCREEN_H_
#define SCREEN_H_



// Class that simply holds the screen dimensions. This is really just a wrapper
// around some global variables.
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
