/* Input.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/Input.h"

#include <SDL2/SDL.h>



// Read the current keyboard state.
const Uint8 *Input::GetState()
{
	static Uint8 array[512];
	return array;
}



SDL_Scancode Input::GetScancodeFromKey(SDL_Keycode keycode)
{
	return static_cast<SDL_Scancode>(0);
}



const char *Input::GetKeyName(SDL_Keycode keycode)
{
	return "";
}



SDL_Keycode Input::GetKeyFromName(const char *name)
{
	return static_cast<SDL_Keycode>(0);
}



SDL_Keymod Input::GetModState()
{
	return static_cast<SDL_Keymod>(0);
}



void Input::GetMouseState(int *x, int *y)
{
}
