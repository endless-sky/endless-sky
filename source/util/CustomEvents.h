/* CustomEvents.h
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <SDL2/SDL_events.h>



// A class containing all custom events we register in SDL.
class CustomEvents {
public:
	static void Init();

	// Get the registered ID of the custom resize event.
	static Uint32 GetResize();
	// Send the custom resize event.
	static void SendResize();
};
