/* MenuPanel.cpp
Michael Zahniser, 5 Nov 2013

Function definitions for the MenuPanel class.
*/

#include "MenuPanel.h"

#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Interface.h"
#include "Information.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "PointerShader.h"
#include "PreferencesPanel.h"
#include "Sprite.h"
#include "SpriteShader.h"

#include <fstream>

using namespace std;

namespace {
	static float alpha = 1.;
	static const int scrollSpeed = 2;
}



MenuPanel::MenuPanel(GameData &gameData, PlayerInfo &playerInfo)
	: gameData(gameData), playerInfo(playerInfo), scroll(0)
{
	ifstream in(gameData.ResourcePath() + "credits.txt");
	string line;
	while(getline(in, line))
		credits.push_back(line);
}



void MenuPanel::Step(bool isActive)
{
	if(isActive && alpha < 1.)
	{
		++scroll;
		if(scroll >= (20 * credits.size() + 300) * scrollSpeed)
			scroll = 0;
	}
}



void MenuPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	gameData.Background().Draw(Point(), Point());
	
	const Ship *player = playerInfo.GetShip();
	Information info;
	info.SetSprite("ship sprite", player->GetSprite().GetSprite());
	info.SetString("pilot", playerInfo.FirstName() + " " + playerInfo.LastName());
	info.SetString("ship", player->Name());
	if(player->GetTargetPlanet() && player->GetTargetPlanet()->GetPlanet())
		info.SetString("planet", player->GetTargetPlanet()->GetPlanet()->Name());
	info.SetString("system", player->GetSystem()->Name());
	info.SetString("credits", to_string(playerInfo.Accounts().Credits()));
	info.SetString("date", playerInfo.GetDate().ToString());
	
	const Interface *menu = gameData.Interfaces().Get("main menu");
	menu->Draw(info);
	
	int progress = static_cast<int>(gameData.Progress() * 60.);
	
	if(progress == 60)
		alpha -= .02f;
	if(alpha > 0.f)
	{
		Angle da(6.);
		Angle a(0.);
		for(int i = 0; i < progress; ++i)
		{
			float color[4] = {alpha, alpha, alpha, 0.f};
			PointerShader::Draw(Point(), a.Unit(), 8., 20., 140. * alpha, color);
			a += da;
		}
	}
	
	const Font &font = FontSet::Get(14);
	int y = 120 - scroll / scrollSpeed;
	for(const string &line : credits)
	{
		float fade = 1.f;
		if(y < -145)
			fade = max(0.f, (y + 165) / 20.f);
		else if(y > 95)
			fade = max(0.f, (115 - y) / 20.f);
		if(fade)
		{
			Color color(((line.empty() || line[0] == ' ') ? .2 : .4) * fade, 0.);
			font.Draw(line, Point(-465., y), color.Get());
		}
		y += 20;
	}
}



bool MenuPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(gameData.Progress() < 1.)
		return false;
	
	if(key == 'e' || key == gameData.Keys().Get(Key::MENU))
		Pop(this);
	else if(key == 'p')
		Push(new PreferencesPanel(gameData));
	else if(key == 'q')
		Quit();
	
	return true;
}



bool MenuPanel::Click(int x, int y)
{
	char key = gameData.Interfaces().Get("main menu")->OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDLKey>(key), KMOD_NONE);
	
	return true;
}
