/* CreditsPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CreditsPanel.h"

#include "Color.h"
#include "Font.h"
#include "FontSet.h"
#include "Point.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

using namespace std;

namespace {
	static const int OFF_X = 200;
	static const int OFF_Y = 0;
}



// Create a panel.
CreditsPanel::CreditsPanel(const string &message, int *amount, int limit)
	: message(message), amount(amount), limit(limit)
{
}



// Draw this panel.
void CreditsPanel::Draw() const
{
	const Sprite *ui = SpriteSet::Get("ui/credits");
	SpriteShader::Draw(ui, Point(OFF_X, OFF_Y));
	
	const Font &font = FontSet::Get(14);
	Color grey(.5, 0.);
	Color bright(.8, 0.);
	
	font.Draw(message, Point(-85. + OFF_X, -35. + OFF_Y), grey);
	
	string amountString = to_string(*amount);
	int amountWidth = font.Width(amountString);
	font.Draw(amountString, Point(75. - amountWidth + OFF_X, -12. + OFF_Y), bright);
	
	static const string cancel = "Cancel";
	static const string okay = "OK";
	int cancelWidth = font.Width(cancel);
	int okayWidth = font.Width(okay);
	
	font.Draw(cancel, Point(-45. - .5 * cancelWidth + OFF_X, 28. + OFF_Y), bright);
	font.Draw(okay, Point(45. - .5 * okayWidth + OFF_X, 28. + OFF_Y),
		(*amount <= limit) ? bright : grey);
}



// Only override the ones you need; the default action is to return false.
bool CreditsPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key >= '0' && key <= '9' && *amount < 4000000000)
		*amount = (*amount * 10) + key - '0';
	else if(key == SDLK_DELETE || key == SDLK_BACKSPACE)
		*amount /= 10;
	else if(key == SDLK_ESCAPE)
	{
		*amount = 0;
		GetUI()->Pop(this);
	}
	else if(key == SDLK_RETURN)
	{
		if(*amount <= limit)
			GetUI()->Pop(this);
		else
			*amount = limit;
	}
	return true;
}



bool CreditsPanel::Click(int x, int y)
{
	x -= OFF_X;
	y -= OFF_Y;
	if(y >= 25 && y < 45)
	{
		if(x >= -75 && x <= -15)
			KeyDown(SDLK_ESCAPE, KMOD_NONE);
		else if(x >= 15 && x <= 75)
			KeyDown(SDLK_RETURN, KMOD_NONE);
	}
	
	return true;
}
