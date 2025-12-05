/* PanelUtils.h
Copyright (c) 2024 by the Endless Sky developers

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

#include <SDL2/SDL.h>

class Command;

namespace PanelUtils {

// Handle zoom key presses. Returns true if key was handled.
// If checkCommand is true, only handles when command is empty (MainPanel behavior).
bool HandleZoomKey(SDL_Keycode key, const Command &command, bool checkCommand = false);

// Handle scroll wheel zoom. Returns true if handled.
bool HandleZoomScroll(double dy);

}
