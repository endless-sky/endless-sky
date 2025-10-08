/* HelpOverlay.cpp
Copyright (c) 2025 by xobes

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HelpOverlay.h"

#include "audio/Audio.h"
#include "text/Font.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "TextArea.h"
#include "UI.h"

using namespace std;



HelpOverlay::HelpOverlay(const string &name)
	: name(name)
{
	Audio::Pause();
	SetInterruptible(false);
	UI::PlaySound(UI::UISound::SOFT);
}



HelpOverlay::~HelpOverlay()
{
	Audio::Resume();
}



void HelpOverlay::Draw()
{
	Information info;

	const Interface *helpUi = GameData::Interfaces().Get(name);
	helpUi->Draw(info, this);
}



bool HelpOverlay::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NONE;

	if(key == 'd' || key == SDLK_ESCAPE || key == SDLK_RETURN || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		GetUI()->Pop(this);
		sound = UI::UISound::SOFT;
	}
	// TODO: F1 pops next help page in series

	UI::PlaySound(sound);
	return true;
}
