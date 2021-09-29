/* Input.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../Input.h"

#include <SDL2/SDL.h>



// Read the current keyboard state.
const Uint8 *Input::GetState()
{
	return SDL_GetKeyboardState(nullptr);
}



SDL_Scancode Input::GetScancodeFromKey(SDL_Keycode keycode)
{
	return SDL_GetScancodeFromKey(keycode);
}



const char *Input::GetKeyName(SDL_Keycode keycode)
{
	return SDL_GetKeyName(keycode);
}



SDL_Keycode Input::GetKeyFromName(const char *name)
{
	return SDL_GetKeyFromName(name);
}


SDL_Keymod Input::GetModState()
{
	return SDL_GetModState();
}



void Input::GetMouseState(int *x, int *y)
{
	SDL_GetMouseState(x, y);
}
