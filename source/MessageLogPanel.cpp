/* MessageLogPanel.cpp
Copyright (c) 2023 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MessageLogPanel.h"

#include "text/alignment.hpp"
#include "Color.h"
#include "Command.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Interface.h"
#include "Preferences.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"
#include "text/WrappedText.h"

using namespace std;

namespace {
	const double PAD = 10.;
	const double LINE_HEIGHT = 25.;
}



MessageLogPanel::MessageLogPanel()
	: messages(Messages::GetLog()), width(GameData::Interfaces().Get("message log")->GetValue("width"))
{
	SetInterruptible(false);
}



void MessageLogPanel::Draw()
{
	// Dim out everything outside this panel.
	DrawBackdrop();

	// Draw the panel.
	const Color &backColor = *GameData::Colors().Get("message log background");
	FillShader::Fill(
		Point(Screen::Left() + .5 * width, 0.),
		Point(width, Screen::Height()),
		backColor);

	Panel::DrawEdgeSprite(SpriteSet::Get("ui/right edge"), Screen::Left() + width);

	const Font &font = FontSet::Get(14);

	// Parameters for drawing messages:
	WrappedText messageLine(font);
	messageLine.SetAlignment(Alignment::LEFT);
	messageLine.SetWrapWidth(width - 2. * PAD);

	// Draw messages.
	Point pos = Screen::BottomLeft() + Point(PAD, scroll);
	for(const auto &it : messages)
	{
		messageLine.Wrap(it.first);
		pos.Y() -= messageLine.Height();
		if(pos.Y() >= Screen::Top() - 3 * font.Height())
			messageLine.Draw(pos, *Messages::GetColor(it.second, true));
	}

	maxScroll = max(0., scroll - pos.Y() + Screen::Top());
}



bool MessageLogPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(command.Has(Command::MESSAGE_LOG) || key == 'd' || key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		double direction = (key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN);
		Drag(0., (Screen::Height() - 100.) * direction);
	}
	else if(key == SDLK_HOME || key == SDLK_END)
	{
		double direction = (key == SDLK_HOME) - (key == SDLK_END);
		Drag(0., maxScroll * direction);
	}
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		double direction = (key == SDLK_UP) - (key == SDLK_DOWN);
		Drag(0., LINE_HEIGHT * direction);
	}

	return true;
}



bool MessageLogPanel::Click(int x, int y, int clicks)
{
	x -= Screen::Left();

	if(x > width)
		GetUI()->Pop(this);

	return true;
}



bool MessageLogPanel::Drag(double dx, double dy)
{
	scroll = max(0., min(maxScroll, scroll + dy));

	return true;
}



bool MessageLogPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}
