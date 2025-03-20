/* Clipboard.h
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

#include <SDL2/SDL_keycode.h>
#include <string>



// A helper interface for SDL functions related to the clipboard functionality.
class Clipboard {
public:
	// Handle keys used for clipboard operations on inputBuffer. Return false if the keys
	// don't have any functionality assigned to them. Optionally, size limit of the input buffer
	// and a set of forbidden characters can be provided.
	static bool KeyDown(std::string &inputBuffer, SDL_Keycode key, Uint16 mod, size_t maxSize = -1,
		const std::string &forbidden = {});


private:
	// Replace the current contents with the provided string.
	static void Set(const std::string &inputBuffer);
	// Get the current clipboard contents, excluding characters we don't want.
	static std::string Get(size_t maxSize, const std::string &forbidden);
};
