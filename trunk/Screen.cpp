/* Screen.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
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
