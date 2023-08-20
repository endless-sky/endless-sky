/* MenuPanel.cpp
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

#include "MenuPanel.h"

#include "Audio.h"
#include "Command.h"
#include "Files.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "LoadPanel.h"
#include "Logger.h"
#include "MainPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "PreferencesPanel.h"
#include "Rectangle.h"
#include "Ship.h"
#include "Sprite.h"
#include "StarField.h"
#include "StartConditionsPanel.h"
#include "System.h"
#include "UI.h"

#include "opengl.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

using namespace std;

namespace {
	const int SCROLL_MOD = 2;
	int scrollSpeed = 1;
	bool showCreditsWarning = true;
}



MenuPanel::MenuPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), mainMenuUi(GameData::Interfaces().Get("main menu"))
{
	assert(GameData::IsLoaded() && "MenuPanel should only be created after all data is fully loaded");
	SetIsFullScreen(true);

	if(mainMenuUi->GetBox("credits").Dimensions())
	{
		for(const auto &source : GameData::Sources())
		{
			auto credit = Format::Split(Files::Read(source + "credits.txt"), "\n");
			if((credit.size() > 1) || (credit.front() != ""))
			{
				credits.insert(credits.end(), credit.begin(), credit.end());
				credits.insert(credits.end(), 15, "");
			}
		}
		// Remove the last 15 lines, as there is already a gap at the beginning of the credits.
		credits.resize(credits.size() - 15);
	}
	else if(showCreditsWarning)
	{
		Logger::LogError("Warning: interface \"main menu\" does not contain a box for \"credits\"");
		showCreditsWarning = false;
	}

	if(gamePanels.IsEmpty())
	{
		gamePanels.Push(new MainPanel(player));
		// It takes one step to figure out the planet panel should be created, and
		// another step to actually place it. So, take two steps to avoid a flicker.
		gamePanels.StepAll();
		gamePanels.StepAll();
	}

	if(player.GetPlanet())
		Audio::PlayMusic(player.GetPlanet()->MusicName());
}



void MenuPanel::Step()
{
	if(GetUI()->IsTop(this) && !scrollingPaused)
	{
		scroll += scrollSpeed;
		if(scroll < 0)
			scroll = (20 * static_cast<long long int>(credits.size()) + 299) * SCROLL_MOD;
		if(scroll >= (20 * static_cast<long long int>(credits.size()) + 300) * SCROLL_MOD)
			scroll = 0;
	}
}



void MenuPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());

	Information info;
	if(player.IsLoaded() && !player.IsDead())
	{
		info.SetCondition("pilot loaded");
		info.SetString("pilot", player.FirstName() + " " + player.LastName());
		if(player.Flagship())
		{
			const Ship &flagship = *player.Flagship();
			info.SetSprite("ship sprite", flagship.GetSprite());
			info.SetString("ship", flagship.Name());
		}
		if(player.GetSystem())
			info.SetString("system", player.GetSystem()->Name());
		if(player.GetPlanet())
			info.SetString("planet", player.GetPlanet()->Name());
		info.SetString("credits", Format::Credits(player.Accounts().Credits()));
		info.SetString("date", player.GetDate().ToString());
		info.SetString("playtime", Format::PlayTime(player.GetPlayTime()));
	}
	else if(player.IsLoaded())
	{
		info.SetCondition("no pilot loaded");
		info.SetString("pilot", player.FirstName() + " " + player.LastName());
		info.SetString("ship", "You have died.");
	}
	else
	{
		info.SetCondition("no pilot loaded");
		info.SetString("pilot", "No Pilot Loaded");
	}

	GameData::Interfaces().Get("menu background")->Draw(info, this);
	mainMenuUi->Draw(info, this);
	GameData::Interfaces().Get("menu player info")->Draw(info, this);

	if(!credits.empty())
		DrawCredits();
}



bool MenuPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(player.IsLoaded() && (key == 'e' || command.Has(Command::MENU)))
	{
		gamePanels.CanSave(true);
		GetUI()->PopThrough(this);
	}
	else if(key == 'p')
		GetUI()->Push(new PreferencesPanel());
	else if(key == 'l')
		GetUI()->Push(new LoadPanel(player, gamePanels));
	else if(key == 'n' && (!player.IsLoaded() || player.IsDead()))
	{
		// If no player is loaded, the "Enter Ship" button becomes "New Pilot."
		// Request that the player chooses a start scenario.
		// StartConditionsPanel also handles the case where there's no scenarios.
		GetUI()->Push(new StartConditionsPanel(player, gamePanels, GameData::StartOptions(), nullptr));
	}
	else if(key == 'q')
		GetUI()->Quit();
	else if(key == ' ')
		scrollingPaused = !scrollingPaused;
	else if(key == SDLK_DOWN)
		scrollSpeed += 1;
	else if(key == SDLK_UP)
		scrollSpeed -= 1;
	else
		return false;

	return true;
}



bool MenuPanel::Click(int x, int y, int clicks)
{
	// Double clicking on the credits pauses/resumes the credits scroll.
	if(clicks == 2 && mainMenuUi->GetBox("credits").Contains(Point(x, y)))
	{
		scrollingPaused = !scrollingPaused;
		return true;
	}

	return false;
}



void MenuPanel::DrawCredits() const
{
	const Font &font = FontSet::Get(14);
	const auto creditsRect = mainMenuUi->GetBox("credits");
	const int top = static_cast<int>(creditsRect.Top());
	const int bottom = static_cast<int>(creditsRect.Bottom());
	int y = bottom + 5 - scroll / SCROLL_MOD;
	for(const string &line : credits)
	{
		float fade = 1.f;
		if(y < top + 20)
			fade = max(0.f, (y - top) / 20.f);
		else if(y > bottom - 20)
			fade = max(0.f, (bottom - y) / 20.f);
		if(fade)
		{
			Color color(((line.empty() || line[0] == ' ') ? .2f : .4f) * fade, 0.f);
			font.Draw(line, Point(creditsRect.Left(), y), color);
		}
		y += 20;
	}
}
