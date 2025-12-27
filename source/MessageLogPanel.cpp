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

#include "text/Alignment.h"
#include "audio/Audio.h"
#include "Color.h"
#include "Command.h"
#include "Dialog.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Preferences.h"
#include "Screen.h"
#include "image/SpriteSet.h"
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
	Audio::Pause();
	SetInterruptible(false);
}



MessageLogPanel::~MessageLogPanel()
{
	Audio::Resume();
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

	Information info;
	if(messages.empty())
		info.SetCondition("empty");
	else
	{
		const Font &font = FontSet::Get(14);

		// Parameters for drawing messages:
		WrappedText messageLine(font);
		messageLine.SetAlignment(Alignment::LEFT);
		messageLine.SetWrapWidth(width - 2. * PAD);

		// Draw messages.
		Point pos = Screen::BottomLeft() + Point(PAD, scroll);
		for(const auto &[text, category] : messages)
		{
			if(importantOnly && !category->IsImportant())
				continue;

			messageLine.Wrap(text);
			pos.Y() -= messageLine.Height();
			if(pos.Y() >= Screen::Top() - 3 * font.Height())
				messageLine.Draw(pos, category->LogColor());
		}

		maxScroll = max(0., scroll - pos.Y() + Screen::Top());
	}

	if(importantOnly)
		info.SetCondition("important messages only");

	GameData::Interfaces().Get("message log")->Draw(info, this);
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
	else if(key == 'i')
		importantOnly = !importantOnly;
	else if(key == 'c' && !messages.empty())
		GetUI()->Push(new Dialog{&Messages::ClearLog, "Clear the message log?", Truncate::NONE, true, false});

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
