/* DebugPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "DebugPanel.h"

#include "audio/Audio.h"
#include "Command.h"
#include "Files.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "GamerulesPanel.h"
#include "Information.h"
#include "Interface.h"
#include "LoadPanel.h"
#include "Logger.h"
#include "MainPanel.h"
#include "pi.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "PreferencesPanel.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "shader/StarField.h"
#include "StartConditionsPanel.h"
#include "System.h"
#include "UI.h"

#include "opengl.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "DialogPanel.h"

using namespace std;



DebugPanel::DebugPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), mainDebugUi(GameData::Interfaces().Get("debug menu"))
{
	assert(GameData::IsLoaded() && "DebugPanel should only be created after all data is fully loaded");
	SetIsFullScreen(true);

	GameData::SetBackgroundPosition(Point());
}



DebugPanel::~DebugPanel()
{
	Audio::Resume();
}



void DebugPanel::Step()
{
	GameData::StepBackground(Point());
}



void DebugPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point());

	Information info;
	mainDebugUi->Draw(info, this);
}



bool DebugPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == 'x')
	{
		GetUI().Push(DialogPanel::Info("Testing DialogPanel::Info.\n\n"
			"The only option should be OK. Esc and Ctrl-W should close the window.\n"
			"There is no callback."));

		// void WarningsDialogCallback(bool isOk);
		// void PlanetPanel::WarningsDialogCallback(const bool isOk)
		// {
		// 	if(isOk)
		// 		TakeOff(true);
		// }
		// GetUI().Push(DialogPanel::CallFunctionOnExit(this,
		// 	&PlanetPanel::WarningsDialogCallback, out.str()));


	}
	UI::PlaySound(UI::UISound::NORMAL);
	return true;
}



bool DebugPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;


	return false;
}