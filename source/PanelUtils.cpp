/* PanelUtils.cpp
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

#include "PanelUtils.h"

#include "Command.h"
#include "Preferences.h"

namespace PanelUtils {

bool HandleZoomKey(SDL_Keycode key, const Command &command, bool checkCommand)
{
	// If checking command, only handle when no command is active
	if(checkCommand && command)
		return false;

	if(key == SDLK_MINUS || key == SDLK_KP_MINUS)
	{
		Preferences::ZoomViewOut();
		return true;
	}
	if(key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS)
	{
		Preferences::ZoomViewIn();
		return true;
	}
	return false;
}



bool HandleZoomScroll(double dy)
{
	if(dy < 0)
		Preferences::ZoomViewOut();
	else if(dy > 0)
		Preferences::ZoomViewIn();
	else
		return false;
	return true;
}

}
