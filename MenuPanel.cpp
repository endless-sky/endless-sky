/* MenuPanel.cpp
Michael Zahniser, 5 Nov 2013

Function definitions for the MenuPanel class.
*/

#include "MenuPanel.h"

#include "GameData.h"
#include "Interface.h"
#include "Information.h"
#include "Point.h"
#include "PointerShader.h"
#include "Sprite.h"
#include "SpriteShader.h"

namespace {
	static float alpha = 1.;
}



MenuPanel::MenuPanel(const GameData &gameData)
	: gameData(gameData)
{
}



void MenuPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	const Interface *menu = gameData.Interfaces().Get("main menu");
	menu->Draw(Information());
	
	gameData.Background().Draw(Point(), Point());
	
	int progress = static_cast<int>(gameData.Progress() * 60.);
	
	if(progress == 60)
		alpha -= .02f;
	if(alpha > 0.f)
	{
		Angle da(6.);
		Angle a(0.);
		for(int i = 0; i < progress; ++i)
		{
			float color[4] = {1.f, 1.f, 1.f, alpha};
			PointerShader::Draw(Point(), a.Unit(), 8., 20. / alpha, 140. / alpha, color);
			a += da;
		}
	}
}



bool MenuPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(gameData.Progress() == 1. && key == SDLK_ESCAPE)
		Pop(this);
	
	return true;
}

