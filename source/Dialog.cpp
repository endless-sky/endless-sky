/* Dialog.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Dialog.h"

#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "DataNode.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "MapDetailPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "shift.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "UI.h"

#include <cmath>
#include <utility>

using namespace std;

namespace {
	const int WIDTH = 250;
	const int WIDE_WIDTH = 450;

	// Map any conceivable numeric keypad keys to their ASCII values. Most of
	// these will presumably only exist on special programming keyboards.
	const map<SDL_Keycode, char> KEY_MAP = {
		{SDLK_KP_0, '0'},
		{SDLK_KP_1, '1'},
		{SDLK_KP_2, '2'},
		{SDLK_KP_3, '3'},
		{SDLK_KP_4, '4'},
		{SDLK_KP_5, '5'},
		{SDLK_KP_6, '6'},
		{SDLK_KP_7, '7'},
		{SDLK_KP_8, '8'},
		{SDLK_KP_9, '9'},
		{SDLK_KP_AMPERSAND, '&'},
		{SDLK_KP_AT, '@'},
		{SDLK_KP_A, 'a'},
		{SDLK_KP_B, 'b'},
		{SDLK_KP_C, 'c'},
		{SDLK_KP_D, 'd'},
		{SDLK_KP_E, 'e'},
		{SDLK_KP_F, 'f'},
		{SDLK_KP_COLON, ':'},
		{SDLK_KP_COMMA, ','},
		{SDLK_KP_DIVIDE, '/'},
		{SDLK_KP_EQUALS, '='},
		{SDLK_KP_EXCLAM, '!'},
		{SDLK_KP_GREATER, '>'},
		{SDLK_KP_HASH, '#'},
		{SDLK_KP_LEFTBRACE, '{'},
		{SDLK_KP_LEFTPAREN, '('},
		{SDLK_KP_LESS, '<'},
		{SDLK_KP_MINUS, '-'},
		{SDLK_KP_MULTIPLY, '*'},
		{SDLK_KP_PERCENT, '%'},
		{SDLK_KP_PERIOD, '.'},
		{SDLK_KP_PLUS, '+'},
		{SDLK_KP_POWER, '^'},
		{SDLK_KP_RIGHTBRACE, '}'},
		{SDLK_KP_RIGHTPAREN, ')'},
		{SDLK_KP_SPACE, ' '},
		{SDLK_KP_VERTICALBAR, '|'}
	};
}



Dialog::Dialog(function<void()> okFunction, const string &message, Truncate truncate, bool canCancel, bool okIsActive)
	: voidFun(okFunction)
{
	Init(message, truncate, canCancel, false);
	this->okIsActive = okIsActive;
}



// Dialog that has no callback (information only). In this form, there is
// only an "ok" button, not a "cancel" button.
Dialog::Dialog(const string &text, Truncate truncate, bool allowsFastForward)
	: allowsFastForward(allowsFastForward)
{
	Init(text, truncate, false);
}



// Mission accept / decline dialog.
Dialog::Dialog(const string &text, PlayerInfo &player, const System *system, Truncate truncate, bool allowsFastForward)
	: intFun(bind(&PlayerInfo::MissionCallback, &player, placeholders::_1)),
	allowsFastForward(allowsFastForward),
	system(system), player(&player)
{
	Init(text, truncate, true, true);
}



// Draw this panel.
void Dialog::Draw()
{
	DrawBackdrop();

	const Sprite *top = SpriteSet::Get(isWide ? "ui/dialog top wide" : "ui/dialog top");
	const Sprite *middle = SpriteSet::Get(isWide ? "ui/dialog middle wide" : "ui/dialog middle");
	const Sprite *bottom = SpriteSet::Get(isWide ? "ui/dialog bottom wide" : "ui/dialog bottom");
	const Sprite *cancel = SpriteSet::Get("ui/dialog cancel");

	// Get the position of the top of this dialog, and of the text and input.
	Point pos(0., (top->Height() + height * middle->Height() + bottom->Height()) * -.5f);
	Point textPos(Width() * -.5 + 10., pos.Y() + 20.);
	Point inputPos = Point(0., -70.) - pos;

	// Draw the top section of the dialog box.
	pos.Y() += top->Height() * .5;
	SpriteShader::Draw(top, pos);
	pos.Y() += top->Height() * .5;

	// The middle section is duplicated depending on how long the text is.
	for(int i = 0; i < height; ++i)
	{
		pos.Y() += middle->Height() * .5;
		SpriteShader::Draw(middle, pos);
		pos.Y() += middle->Height() * .5;
	}

	// Draw the bottom section.
	const Font &font = FontSet::Get(14);
	pos.Y() += bottom->Height() * .5;
	SpriteShader::Draw(bottom, pos);
	pos.Y() += bottom->Height() * .5 - 25.;

	// Draw the buttons, including optionally the cancel button.
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("faint");
	const Color &inactive = *GameData::Colors().Get("inactive");
	if(canCancel)
	{
		string cancelText = isMission ? "Decline" : "Cancel";
		cancelPos = pos + Point(isWide ? 110. : 10., 0.);
		SpriteShader::Draw(cancel, cancelPos);
		Point labelPos(
			cancelPos.X() - .5 * font.Width(cancelText),
			cancelPos.Y() - .5 * font.Height());
		font.Draw(cancelText, labelPos, !okIsActive ? bright : dim);
	}
	string okText = isMission ? "Accept" : "OK";
	okPos = pos + Point(isWide ? 190. : 90., 0.);
	Point labelPos(
		okPos.X() - .5 * font.Width(okText),
		okPos.Y() - .5 * font.Height());
	font.Draw(okText, labelPos, isOkDisabled ? inactive : (okIsActive ? bright : dim));

	// Draw the input, if any.
	if(!isMission && (intFun || stringFun))
	{
		FillShader::Fill(inputPos, Point(Width() - 20., 20.), back);

		Point stringPos(
			inputPos.X() - (Width() - 20) * .5 + 5.,
			inputPos.Y() - .5 * font.Height());
		const auto inputText = DisplayText(input, {Width() - 30, Truncate::FRONT});
		font.Draw(inputText, stringPos, bright);

		Point barPos(stringPos.X() + font.FormattedWidth(inputText) + 2., inputPos.Y());
		FillShader::Fill(barPos, Point(1., 16.), dim);
	}
}



// Format and add the text from the given node to the given string.
void Dialog::ParseTextNode(const DataNode &node, size_t startingIndex, string &text)
{
	for(int i = startingIndex; i < node.Size(); ++i)
	{
		if(!text.empty())
			text += "\n\t";
		text += node.Token(i);
	}
	for(const DataNode &child : node)
		for(int i = 0; i < child.Size(); ++i)
		{
			if(!text.empty())
				text += "\n\t";
			text += child.Token(i);
		}
}



bool Dialog::AllowsFastForward() const noexcept
{
	return allowsFastForward;
}



bool Dialog::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	auto it = KEY_MAP.find(key);
	bool isCloseRequest = key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)));
	if((it != KEY_MAP.end() || (key >= ' ' && key <= '~')) && !isMission && (intFun || stringFun) && !isCloseRequest)
	{
		int ascii = (it != KEY_MAP.end()) ? it->second : key;
		char c = ((mod & KMOD_SHIFT) ? SHIFT[ascii] : ascii);
		// Caps lock should shift letters, but not any other keys.
		if((mod & KMOD_CAPS) && c >= 'a' && c <= 'z')
			c += 'A' - 'a';

		if(stringFun)
			input += c;
		// Integer input should not allow leading zeros.
		else if(intFun && c == '0' && !input.empty())
			input += c;
		else if(intFun && c >= '1' && c <= '9')
			input += c;

		if(validateFun)
			isOkDisabled = !validateFun(input);
	}
	else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && !input.empty())
	{
		input.erase(input.length() - 1);
		if(validateFun)
			isOkDisabled = !validateFun(input);
	}
	else if(key == SDLK_TAB && canCancel)
		okIsActive = !okIsActive;
	else if(key == SDLK_LEFT)
		okIsActive = !canCancel;
	else if(key == SDLK_RIGHT)
		okIsActive = true;
	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER || isCloseRequest
			|| (isMission && (key == 'a' || key == 'd')))
	{
		// Shortcuts for "accept" and "decline."
		if(key == 'a' || (!canCancel && isCloseRequest))
			okIsActive = true;
		if(key == 'd' || (canCancel && isCloseRequest))
			okIsActive = false;
		if(boolFun)
		{
			DoCallback(okIsActive);
			// Use PopThrough because the Dialog has spawned additional panels.
			GetUI()->PopThrough(this);
		}
		else if(okIsActive || isMission)
		{
			// If the OK button is disabled (because the input failed the validation),
			// don't execute the callback.
			if(!isOkDisabled)
			{
				DoCallback();
				GetUI()->PopThrough(this);
			}
		}
		else
			GetUI()->PopThrough(this);
	}
	else if((key == 'm' || command.Has(Command::MAP)) && system && player)
		GetUI()->Push(new MapDetailPanel(*player, system));
	else
		return false;

	return true;
}



bool Dialog::Click(int x, int y, int clicks)
{
	Point clickPos(x, y);

	Point ok = clickPos - okPos;
	if(fabs(ok.X()) < 40. && fabs(ok.Y()) < 20.)
	{
		okIsActive = true;
		return DoKey(SDLK_RETURN);
	}

	if(canCancel)
	{
		Point cancel = clickPos - cancelPos;
		if(fabs(cancel.X()) < 40. && fabs(cancel.Y()) < 20.)
		{
			okIsActive = false;
			return DoKey(SDLK_RETURN);
		}
	}

	return true;
}



// Common code from all three constructors:
void Dialog::Init(const string &message, Truncate truncate, bool canCancel, bool isMission)
{
	SetInterruptible(isMission);

	this->isMission = isMission;
	this->canCancel = canCancel;
	okIsActive = true;
	isWide = false;

	Point textRectSize(Width() - 20, 0);
	text = std::make_shared<TextArea>();
	text->SetAlignment(Alignment::JUSTIFIED);
	text->SetRect(Rectangle(Point(), textRectSize));
	text->SetFont(FontSet::Get(14));
	text->SetTruncate(truncate);
	text->SetText(message);
	AddChild(text);

	// If the dialog is too tall, then switch to wide mode.
	int maxHeight = Screen::Height() * 3 / 4;
	if(text->GetTextHeight() > maxHeight)
	{
		textRectSize.Y() = maxHeight;
		isWide = true;
		// Re-wrap with the new width
		textRectSize.X() = Width() - 20;
		text->SetRect(Rectangle(Point{}, textRectSize));

		if(text->GetLongestLineWidth() <= WIDTH)
		{
			// Formatted text is long and skinny (e.g. scan result dialog). Go back
			// to using the default width, since the wide width doesn't help.
			isWide = false;
			textRectSize.X() = Width() - 20;
			text->SetRect(Rectangle(Point{}, textRectSize));
		}
	}
	else
		textRectSize.Y() = text->GetTextHeight();

	// The dialog with no extenders is 80 pixels tall. 10 pixels at the top and
	// bottom are "padding," but text.Height() over-reports the height by about
	// 6 pixels because it includes its own padding at the bottom. If there is a
	// text input, we need another 20 pixels for it and 10 pixels padding.
	height = 10 + (textRectSize.Y() - 6) + 10 + 30 * (!isMission && (intFun || stringFun));
	// Determine how many 40-pixel extension panels we need.
	if(height <= 80)
		height = 0;
	else
		height = (height - 40) / 40;

	// Now that we know how big we want to render the text, position the text
	// area and add it to the UI.
	const Sprite *top = SpriteSet::Get(isWide ? "ui/dialog top wide" : "ui/dialog top");
	const Sprite *middle = SpriteSet::Get(isWide ? "ui/dialog middle wide" : "ui/dialog middle");
	const Sprite *bottom = SpriteSet::Get(isWide ? "ui/dialog bottom wide" : "ui/dialog bottom");

	// Get the position of the top of this dialog, and of the text and input.
	Point pos(0., (top->Height() + height * middle->Height() + bottom->Height()) * -.5f);
	Point textPos(Width() * -.5 + 10., pos.Y() + 20.);
	// Resize textRectSize to match the visual height of the dialog, which will
	// be rounded up from the actual text height by the number of panels that
	// were added. This helps correctly position the TextArea scroll buttons.
	// The text height was over-reported by 6 pixels, so we add those pixels back for consistency.
	textRectSize.Y() = 60 + height * 40 + 6 - 30 * (!isMission && (intFun || stringFun));

	Rectangle textRect = Rectangle::FromCorner(textPos, textRectSize);
	text->SetRect(textRect);
}



void Dialog::DoCallback(const bool isOk) const
{
	if(isMission)
	{
		if(intFun)
			intFun(okIsActive ? Conversation::ACCEPT : Conversation::DECLINE);

		return;
	}

	if(intFun)
	{
		// Only call the callback if the input can be converted to an int.
		// Otherwise treat this as if the player clicked "cancel."
		try {
			intFun(stoi(input));
		}
		catch(...)
		{
		}
	}

	if(stringFun)
		stringFun(input);

	if(voidFun)
		voidFun();

	if(boolFun)
		boolFun(isOk);
}



int Dialog::Width() const
{
	return isWide ? WIDE_WIDTH : WIDTH;
}
