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

#include "audio/Audio.h"
#include "text/Clipboard.h"
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
#include "TextArea.h"
#include "UI.h"

#include <cmath>
#include <utility>

using namespace std;

namespace {
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

	// The width of the margin on the right/left sides of the dialog. This area is part of the sprite,
	// but shouldn't have any text or other graphics rendered over it. (It's mostly transparent.)
	constexpr double LEFT_MARGIN = 20;
	constexpr double RIGHT_MARGIN = 20;
	constexpr double HORIZONTAL_MARGIN = LEFT_MARGIN + RIGHT_MARGIN;
	// The margin on the right/left sides of the button sprite. The bottom segment also includes a button
	// that uses the same value.
	constexpr double BUTTON_LEFT_MARGIN = 10;
	constexpr double BUTTON_RIGHT_MARGIN = 10;
	constexpr double BUTTON_HORIZONTAL_MARGIN = BUTTON_LEFT_MARGIN + BUTTON_RIGHT_MARGIN;
	// The margin on the top/bottom sides of the button sprite. The bottom segment also includes a button
	// that uses the same value.
	constexpr double BUTTON_TOP_MARGIN = 10;
	constexpr double BUTTON_BOTTOM_MARGIN = 10;
	constexpr double BUTTON_VERTICAL_MARGIN = BUTTON_TOP_MARGIN + BUTTON_BOTTOM_MARGIN;
	// The width of the padding used on the left/right sides of each segment, in pixels.
	constexpr double LEFT_PADDING = 10;
	constexpr double RIGHT_PADDING = 10;
	constexpr double HORIZONTAL_PADDING = RIGHT_PADDING + LEFT_PADDING;
	// The height of the padding used by the top/bottom segment, in pixels.
	constexpr double TOP_PADDING = 10;
	constexpr double BOTTOM_PADDING = 10;
	constexpr double VERTICAL_PADDING = TOP_PADDING + BOTTOM_PADDING;
	// The width of the padding at the beginning/end of an input field.
	constexpr double INPUT_LEFT_PADDING = 5;
	constexpr double INPUT_RIGHT_PADDING = 5;
	constexpr double INPUT_HORIZONTAL_PADDING = INPUT_LEFT_PADDING + INPUT_RIGHT_PADDING;
	// The height of the padding at the top/bottom of an input field.
	constexpr double INPUT_TOP_PADDING = 2;
	constexpr double INPUT_BOTTOM_PADDING = 2;
	constexpr double INPUT_VERTICAL_PADDING = INPUT_TOP_PADDING + INPUT_BOTTOM_PADDING;
	// The height of an input field in pixels.
	constexpr double INPUT_HEIGHT = 20;
}



Dialog::Dialog(function<void()> okFunction, const string &message, Truncate truncate, bool canCancel, int activeButton)
	: voidFun(okFunction)
{
	Init(message, truncate, canCancel, false);
	this->activeButton = activeButton;
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



Dialog::~Dialog()
{
	Audio::Resume();
}



// Draw this panel.
void Dialog::Draw()
{
	DrawBackdrop();

	const Sprite *top = SpriteSet::Get(isWide ? "ui/dialog top wide" : "ui/dialog top");
	const Sprite *middle = SpriteSet::Get(isWide ? "ui/dialog middle wide" : "ui/dialog middle");
	const Sprite *bottom = SpriteSet::Get(isWide ? "ui/dialog bottom wide" : "ui/dialog bottom");
	const Sprite *cancel = SpriteSet::Get("ui/dialog cancel");
	const Sprite *thirdButtonSprite = SpriteSet::Get("ui/wide button");

	// Get the position of the top of this dialog, and of the input.
	Point pos(0., (top->Height() + extensionCount * middle->Height() + bottom->Height()) * -.5);
	Point inputPos = Point(0., -(cancel->Height() + INPUT_HEIGHT)) - pos;

	// Draw the top section of the dialog box.
	pos.Y() += top->Height() * .5;
	SpriteShader::Draw(top, pos);
	pos.Y() += top->Height() * .5;

	// The middle section is duplicated depending on how long the text is.
	for(int i = 0; i < extensionCount; ++i)
	{
		pos.Y() += middle->Height() * .5;
		SpriteShader::Draw(middle, pos);
		pos.Y() += middle->Height() * .5;
	}

	// Draw the bottom section.
	const Font &font = FontSet::Get(14);
	pos.Y() += bottom->Height() * .5;
	SpriteShader::Draw(bottom, pos);
	pos.Y() += (bottom->Height() - cancel->Height()) * .5;

	// Draw the buttons, including optionally the cancel button.
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("faint");
	const Color &inactive = *GameData::Colors().Get("inactive");
	okPos = pos + Point((top->Width() - RIGHT_MARGIN - cancel->Width()) * .5, 0.);
	Point labelPos(
		okPos.X() - .5 * font.Width(okText),
		okPos.Y() - .5 * font.Height());
	font.Draw(okText, labelPos, isOkDisabled ? inactive : (activeButton == 1 ? bright : dim));
	if(canCancel)
	{
		cancelPos = pos + Point(okPos.X() - cancel->Width() + BUTTON_RIGHT_MARGIN, 0.);
		SpriteShader::Draw(cancel, cancelPos);
		labelPos = {
				cancelPos.X() - .5 * font.Width(cancelText),
				cancelPos.Y() - .5 * font.Height()};
		font.Draw(cancelText, labelPos, activeButton == 2 ? bright : dim);

		if(numButtons == 3)
		{
			// Third button, always the left-most button:
			thirdPos = pos + Point(
				cancelPos.X() - (thirdButtonSprite->Width() + cancel->Width()) / 2 + BUTTON_RIGHT_MARGIN, 0.);
			SpriteShader::Draw(thirdButtonSprite, thirdPos);
			labelPos = {
				thirdPos.X() - .5 * font.Width(buttonThree.buttonLabel),
				thirdPos.Y() - .5 * font.Height()};
			font.Draw(buttonThree.buttonLabel, labelPos, activeButton == 3 ? bright : dim);
		}
	}

	// Draw the input, if any.
	if(!isMission && (intFun || stringFun || validateFun))
	{
		FillShader::Fill(inputPos, Point(Width() - HORIZONTAL_PADDING, INPUT_HEIGHT), back);

		Point stringPos(
			inputPos.X() - (Width() - HORIZONTAL_PADDING) * .5 + INPUT_LEFT_PADDING,
			inputPos.Y() - .5 * font.Height());
		const auto inputText = DisplayText(input, {static_cast<int>(Width() - HORIZONTAL_PADDING - INPUT_HORIZONTAL_PADDING),
				Truncate::FRONT});
		font.Draw(inputText, stringPos, bright);

		Point barPos(stringPos.X() + font.FormattedWidth(inputText) + INPUT_TOP_PADDING, inputPos.Y());
		FillShader::Fill(barPos, Point(1., INPUT_HEIGHT - INPUT_VERTICAL_PADDING), dim);
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
	if(stringFun && Clipboard::KeyDown(input, key, mod))
	{
		// Input handled by Clipboard.
	}
	else if((it != KEY_MAP.end() || (key >= ' ' && key <= '~')) && !isMission && (intFun || stringFun) && !isCloseRequest)
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
	else if(key == SDLK_TAB)
		// Round-robin to the right, 3->2->1->3
		activeButton = activeButton == 1 ? numButtons : activeButton - 1;
	else if(key == SDLK_LEFT)
	{
		// To the left, 1->2->3->3
		if(activeButton < numButtons)
			activeButton++;
	}
	else if(key == SDLK_RIGHT)
	{
		// To the right, 3->2->1->1
		if(activeButton > 1)
			activeButton--;
	}
	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE || isCloseRequest
			|| (isMission && (key == 'a' || key == 'd'))
			|| (numButtons == 3 && key == buttonThree.buttonKey))
	{
		// Note: The key shortcuts only work when there is no stringFun defined, else they are being typed out.
		if(key == 'a' || (!canCancel && isCloseRequest))
			activeButton = 1;
		if(key == 'd' || (canCancel && isCloseRequest))
			activeButton = 2;
		if(key == buttonThree.buttonKey && numButtons == 3)
			activeButton = 3;

		// Now that we know what button was selected, process the button press
		if(boolFun)
		{
			DoCallback(activeButton == 1);
			GetUI()->Pop(this);
		}
		else if(activeButton == 1 || isMission)
		{
			// If the OK button is disabled (because the input failed the validation),
			// don't execute the callback.
			if(!isOkDisabled)
			{
				DoCallback();
				GetUI()->Pop(this);
			}
		}
		else if(activeButton == 3)
		{
			// Do third button callback. If this returns true, also close the dialog.
			if(buttonThree.buttonAction && buttonThree.buttonAction(input))
				GetUI()->Pop(this);
		}
		else
			GetUI()->Pop(this);
	}
	else if((key == 'm' || command.Has(Command::MAP)) && system && player)
		GetUI()->Push(new MapDetailPanel(*player, system, true));
	else
		return false;

	return true;
}



bool Dialog::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;
	Point clickPos(x, y);

	const Sprite *sprite = SpriteSet::Get("ui/dialog cancel");
	double toleranceX = (sprite->Width() - BUTTON_HORIZONTAL_MARGIN) / 2.;
	double toleranceY = (sprite->Height() - BUTTON_VERTICAL_MARGIN) / 2.;

	Point ok = clickPos - okPos;
	if(fabs(ok.X()) < toleranceX && fabs(ok.Y()) < toleranceY)
	{
		activeButton = 1;
		return DoKey(SDLK_RETURN);
	}

	if(canCancel)
	{
		Point cancel = clickPos - cancelPos;
		if(fabs(cancel.X()) < toleranceX && fabs(cancel.Y()) < toleranceY)
		{
			activeButton = 2;
			return DoKey(SDLK_RETURN);
		}
	}

	if(numButtons == 3)
	{
		Point cancel = clickPos - thirdPos;
		const Sprite *sprite3 = SpriteSet::Get("ui/wide button");
		toleranceX = (sprite3->Width() - BUTTON_HORIZONTAL_MARGIN) / 2.;
		toleranceY = (sprite3->Height() - BUTTON_VERTICAL_MARGIN) / 2.;
		if(fabs(cancel.X()) < toleranceX && fabs(cancel.Y()) < toleranceY)
		{
			activeButton = 3;
			return DoKey(SDLK_RETURN);
		}
	}

	return true;
}



void Dialog::Resize()
{
	isWide = false;
	Point textRectSize(Width() - HORIZONTAL_PADDING, 0);
	text->SetRect(Rectangle(Point(), textRectSize));
	const Sprite *top = SpriteSet::Get("ui/dialog top");
	// If the dialog is too tall, then switch to wide mode.
	int maxHeight = Screen::Height() * 3 / 4;
	if(text->GetTextHeight(false) > maxHeight)
	{
		textRectSize.Y() = maxHeight;
		isWide = true;
		// Re-wrap with the new width
		textRectSize.X() = Width() - HORIZONTAL_PADDING;
		text->SetRect(Rectangle(Point{}, textRectSize));

		if(text->GetLongestLineWidth() <= top->Width() - HORIZONTAL_MARGIN - HORIZONTAL_PADDING)
		{
			// Formatted text is long and skinny (e.g. scan result dialog). Go back
			// to using the default width, since the wide width doesn't help.
			isWide = false;
			textRectSize.X() = Width() - HORIZONTAL_PADDING;
			text->SetRect(Rectangle(Point{}, textRectSize));
		}
	}
	else
		textRectSize.Y() = text->GetTextHeight(false);

	top = SpriteSet::Get(isWide ? "ui/dialog top wide" : "ui/dialog top");
	const Sprite *middle = SpriteSet::Get(isWide ? "ui/dialog middle wide" : "ui/dialog middle");
	const Sprite *bottom = SpriteSet::Get(isWide ? "ui/dialog bottom wide" : "ui/dialog bottom");
	const Sprite *cancel = SpriteSet::Get("ui/dialog cancel");
	// The height of the bottom sprite without the included button's height.
	const int realBottomHeight = bottom->Height() - cancel->Height();

	int height = TOP_PADDING + textRectSize.Y() + BOTTOM_PADDING +
			(realBottomHeight - BOTTOM_PADDING) * (!isMission && (intFun || stringFun));
	// Determine how many extension panels we need.
	if(height <= realBottomHeight + top->Height())
		extensionCount = 0;
	else
		extensionCount = (height - middle->Height()) / middle->Height();

	// Now that we know how big we want to render the text, position the text
	// area and add it to the UI.

	// Get the position of the top of this dialog, and of the text and input.
	Point pos(0., (top->Height() + extensionCount * middle->Height() + bottom->Height()) * -.5f);
	Point textPos(Width() * -.5 + LEFT_PADDING, pos.Y() + VERTICAL_PADDING);
	// Resize textRectSize to match the visual height of the dialog, which will
	// be rounded up from the actual text height by the number of panels that
	// were added. This helps correctly position the TextArea scroll buttons.
	textRectSize.Y() = (top->Height() + realBottomHeight - VERTICAL_PADDING) + extensionCount * middle->Height() -
			(realBottomHeight - BOTTOM_PADDING) * (!isMission && (intFun || stringFun));

	Rectangle textRect = Rectangle::FromCorner(textPos, textRectSize);
	text->SetRect(textRect);
}



// Common code from all three constructors:
void Dialog::Init(const string &message, Truncate truncate, bool canCancel, bool isMission)
{
	Audio::Pause();
	SetInterruptible(isMission);

	this->isMission = isMission;
	this->canCancel = canCancel;
	activeButton = 1;
	isWide = false;
	numButtons = canCancel ? (!buttonThree.buttonLabel.empty() ? 3 : 2) : 1;

	if(buttonOne.buttonLabel.empty())
		okText = isMission ? "Accept" : "OK";
	else
	{
		okText = buttonOne.buttonLabel;
		stringFun = buttonOne.buttonAction;
	}
	cancelText = isMission ? "Decline" : "Cancel";

	text = make_shared<TextArea>();
	text->SetAlignment(Alignment::JUSTIFIED);
	text->SetFont(FontSet::Get(14));
	text->SetTruncate(truncate);
	text->SetText(message);
	Resize();
	AddChild(text);

	if(validateFun)
		isOkDisabled = !validateFun(input);
}



void Dialog::DoCallback(const bool isOk) const
{
	if(isMission)
	{
		if(intFun)
			intFun(activeButton == 1 ? Conversation::ACCEPT : Conversation::DECLINE);

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
	const Sprite *top = SpriteSet::Get(isWide ? "ui/dialog top wide" : "ui/dialog top");
	return top->Width() - HORIZONTAL_MARGIN;
}
