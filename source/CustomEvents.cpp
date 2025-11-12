/* CustomEvents.cpp
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

#include "CustomEvents.h"

#include <cassert>

namespace {
	Uint32 resize = -1;
}



void CustomEvents::Init()
{
	resize = SDL_RegisterEvents(1);
}



Uint32 CustomEvents::GetResize()
{
	assert(resize != static_cast<Uint32>(-1) && "Custom events must be registered");
	return resize;
}



void CustomEvents::SendResize()
{
	SDL_Event event;
	event.type = GetResize();
	SDL_PushEvent(&event);
}
