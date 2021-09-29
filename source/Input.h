/* Input.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <cstdint>

#include <SDL2/SDL.h>



namespace Input
{
	// Returns an array of the current keyboard state.
	const Uint8 *GetState();
	SDL_Scancode GetScancodeFromKey(SDL_Keycode key);
	const char *GetKeyName(SDL_Keycode keycode);
	SDL_Keycode GetKeyFromName(const char *name);
	SDL_Keymod GetModState();
	void GetMouseState(int *x, int *y);
}



#endif
