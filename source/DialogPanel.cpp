/* DialogPanel.cpp
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

#include "DialogPanel.h"

#include "audio/Audio.h"
#include "text/Clipboard.h"
#include "Color.h"
#include "Command.h"
#include "text/DisplayText.h"
#include "Endpoint.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "MapDetailPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Preferences.h"
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
}


DialogPanel::FunctionButton::FunctionButton(const std::string &buttonLabel, SDL_Keycode buttonKey,
	std::function<bool(const std::string &)> buttonAction,
	std::function<bool(const std::string &)> validateFun)
	: buttonLabel(buttonLabel),
	buttonKey(buttonKey),
	buttonAction(std::move(buttonAction)),
	validateStringFun(std::move(validateFun))
{
}



DialogPanel::FunctionButton::FunctionButton(const std::string &buttonLabel, SDL_Keycode buttonKey,
	std::function<bool(const std::string &)> buttonAction)
	: buttonLabel(buttonLabel),
	buttonKey(buttonKey),
	buttonAction(std::move(buttonAction))
{
}



DialogPanel::FunctionButton::FunctionButton(const std::string &buttonLabel, SDL_Keycode buttonKey,
    std::function<bool(std::string *input, int *activeButton)> buttonActionPtr)
	: buttonLabel(buttonLabel),
	buttonKey(buttonKey),
	buttonActionPtr(std::move(buttonActionPtr))
{
}



DialogPanel::~DialogPanel()
{
	Audio::Resume();
}



DialogPanel *DialogPanel::Info(std::string message, Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	init.closeButton = 1;
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	init.hasInputType = NONE;
	return new DialogPanel(init);
}



DialogPanel *DialogPanel::CallFunctionIfOk(std::function<void()> okFunction, std::string message,
	int activeButton, Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.activeButton = activeButton;
	init.message = std::move(message);
	init.buttonOne.voidFun = std::move(okFunction);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	init.hasInputType = NONE;
	return new DialogPanel(init);
}



DialogPanel *DialogPanel::MissionOfferDialog(std::string message, PlayerInfo &player, const System *system,
	Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	init.system = system;
	init.player = &player;
	init.closeButton = 2;  // Ensure that window close calls Decline function
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	init.isInterruptable = true;
	init.hasInputType = NONE;
	init.buttonOne.buttonLabel = "Accept";
	init.buttonOne.buttonKey = 'a';
	init.buttonOne.buttonAction = [&](const std::string &) -> bool
	{
		player.MissionCallback(Endpoint::ACCEPT);
		return true;
	};
	init.buttonTwo.buttonLabel = "Decline";
	init.buttonTwo.buttonKey = 'd';
	init.buttonTwo.buttonAction = [&](const std::string &) -> bool
	{
		player.MissionCallback(Endpoint::DECLINE);
		return true;
	};
	return new DialogPanel(init);
}



// Draw this panel.
void DialogPanel::Draw()
{
	DrawBackdrop();

	const Sprite *top = SpriteSet::Get(isWide ? "ui/dialog top wide" : "ui/dialog top");
	const Sprite *middle = SpriteSet::Get(isWide ? "ui/dialog middle wide" : "ui/dialog middle");
	const Sprite *bottom = SpriteSet::Get(isWide ? "ui/dialog bottom wide" : "ui/dialog bottom");
	const Sprite *buttonSprite = SpriteSet::Get("ui/dialog cancel");
	const Sprite *wideButtonSprite = SpriteSet::Get("ui/wide button");

	// Get the position of the top of this dialog, and of the input.
	Point pos(0., (top->Height() + extensionCount * middle->Height() + bottom->Height()) * -.5);
	Point inputPos = Point(0., -(buttonSprite->Height() + INPUT_HEIGHT)) - pos;

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
	pos.Y() += (bottom->Height() - buttonSprite->Height()) * .5;

	// Draw the buttons.
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("faint");
	const Color &inactive = *GameData::Colors().Get("inactive");
	int lastX = pos.X() + (top->Width() - RIGHT_MARGIN) * .5;
	for(int b = 0; b < numButtons; ++b)
	{
		int d;
		if(b < 1)
			d = buttonSprite->Width() / 2 + BUTTON_RIGHT_MARGIN;
		else if(b < 2)
			d = buttonSprite->Width();
		else if(b < 3)
			d = (wideButtonSprite->Width() + buttonSprite->Width()) / 2;
		else
			d = wideButtonSprite->Width();

		buttonPos[b] = pos + Point(lastX - d + BUTTON_RIGHT_MARGIN, 0.);
		if(b < 2)
			SpriteShader::Draw(buttonSprite, buttonPos[b]);
		else
			SpriteShader::Draw(wideButtonSprite, buttonPos[b]);
		Point labelPos(
			buttonPos[b].X() - .5 * font.Width(buttonList[b].buttonLabel),
			buttonPos[b].Y() - .5 * font.Height());
		// TODO: better active + disabled button look, e.g highlighting
		// TODO: toda: button font; shold also do border,
		// TODO: what to do if active button becomes disabled while typing and then reenabled?
		font.Draw(buttonList[b].buttonLabel, labelPos, isButtonDisabled[b] ?
			inactive : activeButton == b + 1 ? bright : dim);

		lastX = buttonPos[b].X();
	}

	// Draw the input, if any.
	if(AcceptsInput())
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



bool DialogPanel::AllowsFastForward() const noexcept
{
	return allowsFastForward;
}



void DialogPanel::UpdateTextDisplay()
{
	text->SetAlignment(Preferences::GetTextAlignment());
}



void DialogPanel::DialogInit::Ready() {
	// Re-implement legacy Dialog Button behaviors in the function buttons

	// Default OK Button
	if(buttonOne.buttonLabel.empty())
		buttonOne.buttonLabel = "OK";

	// Default Cancel Button,
	// Note: we are using the closeButton (which has a default of 2) to effectively also make the
	//       default configuration into a 2-button Dialog with a Cancel button.
	if(closeButton > 1 && buttonTwo.buttonLabel.empty())
		buttonTwo = DialogPanel::CANCEL_BUTTON;

	// When to display the text entry field:
	if(hasInputType != NONE)
	{
		if(buttonOne.intFun)
			hasInputType = INTEGER;
		if(buttonOne.doubleFun)
			hasInputType = DOUBLE;
		if(buttonOne.stringFun)
			hasInputType = STRING;
	}
}



DialogPanel::DialogPanel(DialogInit &init)
{
	init.Ready();

	allowsFastForward = init.allowsFastForward;
	input = std::move(init.initialValue);
	hasTextEntry = init.hasInputType;
	buttonList[0] = std::move(init.buttonOne);
	buttonList[1] = std::move(init.buttonTwo);
	buttonList[2] = std::move(init.buttonThree);
	buttonList[3] = std::move(init.buttonFour);
	activeButton = init.activeButton;
	closeButton = init.closeButton;
	minHeight = init.minHeight;
	system = init.system;
	player = init.player;
	Audio::Pause();
	SetInterruptible(init.isInterruptable);

	numButtons = buttonList[3].buttonLabel.empty() ?
				(buttonList[2].buttonLabel.empty() ?
				(buttonList[1].buttonLabel.empty() ? 1 : 2) : 3) : 4;

	isWide = forceWide = init.forceWide || numButtons > 3;

	text = make_shared<TextArea>();
	text->SetAlignment(Preferences::GetTextAlignment());
	text->SetFont(FontSet::Get(14));
	text->SetTruncate(init.truncate);
	text->SetText(init.message);
	extensionCount = 0;
	AddChild(text);

	OnInputChange();
}



bool DialogPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_UNKNOWN && !command)
		return false;

	bool isCloseRequest = key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)));

	// Handle changes to the input field:
	auto it = KEY_MAP.find(key);
	if(hasTextEntry == STRING && Clipboard::KeyDown(input, key, mod))
	{
		// Input handled by Clipboard.
		OnInputChange();
	}
	else if((it != KEY_MAP.end() || (key >= ' ' && key <= '~')) && AcceptsInput() && !isCloseRequest)
	{
		int ascii = (it != KEY_MAP.end()) ? it->second : key;
		char c = ((mod & KMOD_SHIFT) ? SHIFT[ascii] : ascii);
		// Caps lock should shift letters, but not any other keys.
		if((mod & KMOD_CAPS) && c >= 'a' && c <= 'z')
			c += 'A' - 'a';

		if(hasTextEntry == STRING)
			input += c;
		// Integer and double inputs only allow certain characters.
		else if((hasTextEntry == INTEGER || hasTextEntry == DOUBLE) && c >= '0' && c <= '9')
			input += c;
		// Both integer and double input can start with a minus sign.
		else if((hasTextEntry == INTEGER || hasTextEntry == DOUBLE) && c == '-' && input.empty())
			input += c;
		// Double input should only allow a single decimal point.
		else if(hasTextEntry == DOUBLE && c == '.' && !std::count(input.begin(), input.end(), '.'))
			input += c;

		OnInputChange();
	}
	else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && !input.empty())
	{
		input.erase(input.length() - 1);
		OnInputChange();
	}

	// Handle toggling between buttons:
	else if(key == SDLK_TAB)
	{
		// TODO: button tabbing needs to skip disabled buttons
		//  while activeButton is disabled... +/-; protect against all buttons inactive
		if(mod & KMOD_SHIFT)
			// Shift + Tab: Round-robin to the left, 1->2->3->1
			activeButton = (activeButton == numButtons) ? 1 : activeButton + 1;
		else
			// Tab: Round-robin to the right, 3->2->1->3
			activeButton = activeButton == 1 ? numButtons : activeButton - 1;
	}

	// Handle button presses:
 	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE || isCloseRequest
			|| key == buttonList[0].buttonKey
			|| (numButtons >= 2 && key == buttonList[1].buttonKey)
			|| (numButtons >= 3 && key == buttonList[2].buttonKey)
			|| (numButtons == 4 && key == buttonList[3].buttonKey))
	{
		// Note: The key shortcuts for buttons only work when AcceptsInput() is false,
		//       otherwise they are being typed out.
		if(key == buttonList[0].buttonKey)
			activeButton = 1;
		else if(numButtons > 1 && key == buttonList[1].buttonKey)
			activeButton = 2;
		else if(numButtons > 2 && key == buttonList[2].buttonKey)
			activeButton = 3;
		else if(numButtons > 3 && key == buttonList[3].buttonKey)
			activeButton = 4;

		if(isCloseRequest || (activeButton == 1 && !HasAction(0)))
		{
			// When close button is defined then close button action shall be called on close.
			// No validation will be performed.
			// When the closeButton is not defined (0) then no button actions will be called,
			// but rather the Dialog will simply be closed.
			// E.g.: "OK" button has no action on close (e.g.: Esc.)
			if(closeButton && HasAction(closeButton - 1))
				DoCallback(closeButton - 1);
			GetUI().Pop(this);
		}
		else if(activeButton <= numButtons)
		{
			int b = activeButton - 1;
			// If the button is disabled (because the input failed the validation),
			// don't execute the callback.
			if(!isButtonDisabled[b] && HasAction(b))
			{
				if(DoCallback(b))
					GetUI().Pop(this);
			}
			else
			{
				UI::PlaySound(UI::UISound::FAILURE);
				return false;
			}
		}
 		// activeButton is out of range, do nothing.
	}

	// Special case for mission Dialogs, I presume
	else if((key == 'm' || command.Has(Command::MAP)) && system && player)
		GetUI().Push(new MapDetailPanel(*player, system, true));

	// Not handled
	else
		return false;

	return true;
}



bool DialogPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;
	Point clickPos(x, y);

	// Buttons 1 and 2 use medium width `dialog cancel`, 3 & 4 use `wide button`
	const Sprite *sprite = SpriteSet::Get("ui/dialog cancel");
	const Sprite *sprite3 = SpriteSet::Get("ui/wide button");

	double toleranceX = (sprite->Width() - BUTTON_HORIZONTAL_MARGIN) / 2.;
	double toleranceY = (sprite->Height() - BUTTON_VERTICAL_MARGIN) / 2.;
	for(int b = 0; b < numButtons; ++b)
	{
		if(b >= 2)
			toleranceX = (sprite3->Width() - BUTTON_HORIZONTAL_MARGIN) / 2.;

		Point delta = clickPos - buttonPos[b];
		if(fabs(delta.X()) < toleranceX && fabs(delta.Y()) < toleranceY)
		{
			activeButton = b + 1;
			return DoKey(SDLK_RETURN);
		}
	}

	return true;
}



bool DialogPanel::HasAction(int b) const
{
	return (buttonList[b].voidFun
		|| buttonList[b].boolFun
		|| buttonList[b].intFun
		|| buttonList[b].doubleFun
		|| buttonList[b].stringFun
		|| buttonList[b].buttonAction
		|| buttonList[b].buttonActionPtr);
}



bool DialogPanel::DoCallback(int b)
{
	if(buttonList[b].intFun)
	{
		// Only call the callback if the input can be converted to an int.
		// Otherwise treat this as if the player clicked "cancel."
		try {
			buttonList[b].intFun(stoi(input));
		}
		catch(...)
		{
		}
	}

	if(buttonList[b].doubleFun)
	{
		// Only call the callback if the input can be converted to a double.
		// Otherwise treat this as if the player clicked "cancel."
		try {
			buttonList[b].doubleFun(stod(input));
		}
		catch(...)
		{
		}
	}

	if(buttonList[b].stringFun)
		buttonList[b].stringFun(input);

	if(buttonList[b].voidFun)
		buttonList[b].voidFun();

	if(buttonList[b].boolFun)
		buttonList[b].boolFun(activeButton == 1);

	if(buttonList[b].buttonActionPtr)
		return buttonList[b].buttonActionPtr(&input, &activeButton);

	if(buttonList[b].buttonAction)
		return buttonList[b].buttonAction(input);

	return true;
}



void DialogPanel::Resize()
{
	Point textRectSize(Width() - HORIZONTAL_PADDING, 0);
	text->SetRect(Rectangle(Point{}, textRectSize));
	const Sprite *top = SpriteSet::Get("ui/dialog top");
	// If the dialog is too tall, then switch to wide mode.
	int maxHeight = Screen::Height() * 3 / 4;
	if(forceWide || text->GetTextHeight(false) > maxHeight)
	{
		isWide = true;
		// Re-wrap with the new width
		textRectSize.X() = Width() - HORIZONTAL_PADDING;
		if(text->GetTextHeight(false) > maxHeight)
			textRectSize.Y() = maxHeight;
		text->SetRect(Rectangle(Point{}, textRectSize));

		if(!forceWide && text->GetLongestLineWidth() <= top->Width() - HORIZONTAL_MARGIN - HORIZONTAL_PADDING)
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

	// A negative height (default) will allow dynamic sizing.
	int height = minHeight;
	if(height < 0)
		height = TOP_PADDING + textRectSize.Y() + BOTTOM_PADDING +
			(realBottomHeight - BOTTOM_PADDING) * AcceptsInput();
	// Determine how many extension panels we need.
	if(height <= realBottomHeight + top->Height() - TOP_PADDING - BOTTOM_PADDING)
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
			realBottomHeight * AcceptsInput() - BOTTOM_PADDING;

	textRect = Rectangle::FromCorner(textPos, textRectSize);
	text->SetRect(textRect);
}



int DialogPanel::Width() const
{
	const Sprite *top = SpriteSet::Get(isWide ? "ui/dialog top wide" : "ui/dialog top");
	return top->Width() - HORIZONTAL_MARGIN;
}



bool DialogPanel::AcceptsInput() const
{
	return hasTextEntry != NONE;
}



bool DialogPanel::ValidateInput(int b) const
{
	if(buttonList[b].validateStringFun)
		return buttonList[b].validateStringFun(input);

	try {
		if(buttonList[b].validateIntFun)
			return buttonList[b].validateIntFun(stoi(input));
		if(buttonList[b].validateDoubleFun)
			return buttonList[b].validateDoubleFun(stod(input));
	}
	catch(...)
	{
		return false;
	}

	return true;
}



// Update the state of the buttons each time the input changes
void DialogPanel::OnInputChange()
{
	for(int b = 0; b < numButtons; ++b)
		isButtonDisabled[b] = !ValidateInput(b);
}
