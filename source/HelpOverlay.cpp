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
#include "Planet.h"
#include "PlayerInfo.h"
#include "Port.h"
#include "System.h"
#include "TextArea.h"
#include "UI.h"

using namespace std;



HelpOverlay::HelpOverlay(PlayerInfo *player, const string &name)
	: name(name), player(*player)
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

	// Some help relies on planet service information, update info based on that data.
	const Planet *planet = player.GetPlanet();
	const System *system = player.GetSystem();
	if(planet->CanUseServices())
	{
		const Port &port = planet->GetPort();

		if(port.HasService(Port::ServicesType::Bank))
			info.SetCondition("has bank");
		if(port.HasService(Port::ServicesType::JobBoard))
			info.SetCondition("has job board");
		if(port.HasService(Port::ServicesType::HireCrew))
			info.SetCondition("can hire crew");
		if(port.HasService(Port::ServicesType::Trading) && system->HasTrade())
			info.SetCondition("has trade");
		if(planet->HasNamedPort())
			info.SetCondition("has port");
		if(planet->HasShipyard())
			info.SetCondition("has shipyard");
		if(planet->HasOutfitter())
			info.SetCondition("has outfitter");
	}

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
