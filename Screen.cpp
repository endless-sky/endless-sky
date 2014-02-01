/* Screen.cpp
Michael Zahniser, 16 Oct 2013

Function and object definitions for the Screen class.
*/

#include "Screen.h"



void Screen::Set(int width, int height)
{
	Screen::width = width;
	Screen::height = height;
}



int Screen::Width()
{
	return width;
}



int Screen::Height()
{
	return height;
}



int Screen::width = 1000;
int Screen::height = 1000;
