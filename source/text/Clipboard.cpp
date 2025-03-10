/* Clipboard.cpp
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

#include "Clipboard.h"

#include <SDL2/SDL_clipboard.h>

using namespace std;



bool Clipboard::KeyDown(string &inputBuffer, SDL_Keycode key, Uint16 mod, size_t maxSize, const string &forbidden)
{
	if(!(mod & KMOD_CTRL))
		return false;

	if(key == 'c')
		Set(inputBuffer);
	else if(key == 'x')
	{
		Set(inputBuffer);
		inputBuffer.clear();
	}
	else if(key == 'v')
		inputBuffer += Get(maxSize - inputBuffer.size(), forbidden);
	else
		return false;

	return true;
}



void Clipboard::Set(const string &text)
{
	SDL_SetClipboardText(text.c_str());
}



string Clipboard::Get(size_t maxSize, const string &forbidden)
{
	if(!SDL_HasClipboardText())
		return "";

	string clipboardString;
	char *clipboardBuffer = SDL_GetClipboardText();
	size_t i = 0;
	for(char *c = clipboardBuffer; *c && i < maxSize; ++c, ++i)
		if(*c >= ' ' && *c <= '~' && forbidden.find(*c) == string::npos)
			clipboardString += *c;
	SDL_free(clipboardBuffer);
	return clipboardString;
}
