/* Dialog.h
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

#pragma once

#include "Panel.h"

#include "Point.h"
#include "text/Truncate.h"

#include <functional>
#include <string>

class DataNode;
class PlayerInfo;
class System;
class TextArea;



namespace dialog {
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



// A dialog box displays a given message to the player. The box will expand to
// fit the message, and may also include a text input field. The box may have
// only an "ok" button, or may also have a "cancel" button. If this dialog is
// introducing a mission, the buttons are instead "accept" and "decline". A
// callback function can be given to receive the player's response.
class Dialog : public Panel {
public:
	class FunctionButton
	{
	public:
		FunctionButton() = default;

		~FunctionButton() = default;

		template<class T>
		FunctionButton(T *panel,
			const std::string &buttonLabel,
			SDL_Keycode buttonKey = '\0',
			bool (T::*buttonAction)(const std::string&) = nullptr);

	public:
		std::string buttonLabel;
		SDL_Keycode buttonKey = '\0';
		std::function<bool(const std::string &)> buttonAction;
	};


public:
	// An OK / Cancel dialog where Cancel can be disabled. The activeButton == 1 lets
	// you select whether "OK" (true) or "Cancel" (false) are selected as the default option.
	Dialog(std::function<void()> okFunction, const std::string &message, Truncate truncate,
		bool canCancel, int activeButton);
	// Dialog that has no callback (information only). In this form, there is
	// only an "ok" button, not a "cancel" button.
	explicit Dialog(const std::string &text, Truncate truncate = Truncate::NONE, bool allowsFastForward = false);
	// Mission accept / decline dialog.
	Dialog(const std::string &text, PlayerInfo &player, const System *system = nullptr,
		Truncate truncate = Truncate::NONE, bool allowsFastForward = false);
	virtual ~Dialog() override;

	// Three different kinds of dialogs can be constructed: requesting numerical
	// input, requesting text input, or not requesting any input at all. In any
	// case, the callback is called only if the user selects "ok", not "cancel."
	template<class T>
	Dialog(T *t, void (T::*fun)(int), const std::string &text,
		Truncate truncate = Truncate::NONE, bool allowsFastForward = false);
	template<class T>
	Dialog(T *t, void (T::*fun)(int), const std::string &text, int initialValue,
		Truncate truncate = Truncate::NONE, bool allowsFastForward = false);

	template<class T>
	Dialog(T *t, void (T::*fun)(const std::string &), const std::string &text, std::string initialValue = "",
		Truncate truncate = Truncate::NONE, bool allowsFastForward = false);

	// This callback requests text input but with validation. The "ok" button is disabled
	// if the validation callback returns false.
	template<class T>
	Dialog(T *t, void (T::*fun)(const std::string &), const std::string &text,
			std::function<bool(const std::string &)> validate,
			std::string initialValue = "",
			Truncate truncate = Truncate::NONE,
			bool allowsFastForward = false);

	template<class T>
	Dialog(T *t, void (T::*fun)(), const std::string &text,
		Truncate truncate = Truncate::NONE, bool allowsFastForward = false);

	// Callback is always called with value user input to dialog (ok == true, cancel == false).
	template<class T>
	Dialog(T *t, void (T::*fun)(bool), const std::string &text,
		Truncate truncate = Truncate::NONE, bool allowsFastForward = false);

	// Three button context, must provide actions for button 1 and button 3, button 2 is cancel.
	template<class T>
	Dialog(T *panel,
		const std::string &text,
		const std::string &initialValue,
		Dialog::FunctionButton buttonOne,
		Dialog::FunctionButton buttonThree,
		std::function<bool(const std::string &)> validate);

	// Draw this panel.
	virtual void Draw() override;

	// Static method used to convert a DataNode into formatted Dialog text.
	static void ParseTextNode(const DataNode &node, size_t startingIndex, std::string &text);

	// Some dialogs allow fast-forward to stay active.
	bool AllowsFastForward() const noexcept final;


protected:
	// The user can click "ok" or "cancel", or use the tab key to toggle which
	// button is highlighted and the enter key to select it.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;

	virtual void Resize() override;
	void Resize(int);

	// Common code from all three constructors:
	void Init(const std::string &message, Truncate truncate, bool canCancel = true, bool isMission = false);
	// The width of the dialog, excluding margins.
	int Width() const;


private:
	void DoCallback(bool isOk = true) const;


protected:
	std::shared_ptr<TextArea> text;
	// The number of extra segments in this dialog.
	int extensionCount;

	std::function<void(int)> intFun;
	std::function<void(const std::string &)> stringFun;
	std::function<void()> voidFun;
	std::function<void(bool)> boolFun;
	std::function<bool(const std::string &)> validateFun;

	bool canCancel;
	int activeButton;
	bool isMission;
	bool isOkDisabled = false;
	bool allowsFastForward = false;
	bool isWide = false;
	Rectangle textRect;

	std::string input;

	std::string okText;
	std::string cancelText;

	Point okPos;
	Point cancelPos;
	Point thirdPos;

	Dialog::FunctionButton buttonOne;
	Dialog::FunctionButton buttonThree;

	int numButtons;

	const System *system = nullptr;
	PlayerInfo *player = nullptr;
};



template<class T>
Dialog::FunctionButton::FunctionButton(T *panel,
	const std::string &buttonLabel,
	SDL_Keycode buttonKey,
	bool(T::*buttonAction)(const std::string &))
	:
	buttonLabel(buttonLabel),
	buttonKey(buttonKey),
	buttonAction(std::bind(buttonAction, panel, std::placeholders::_1))
{
}



template<class T>
Dialog::Dialog(T *t, void (T::*fun)(int), const std::string &text, Truncate truncate, bool allowsFastForward)
	: intFun(std::bind(fun, t, std::placeholders::_1)), allowsFastForward(allowsFastForward)
{
	Init(text, truncate);
}



template<class T>
Dialog::Dialog(T *t, void (T::*fun)(int), const std::string &text,
	int initialValue, Truncate truncate, bool allowsFastForward)
	: intFun(std::bind(fun, t, std::placeholders::_1)),
	allowsFastForward(allowsFastForward),
	input(std::to_string(initialValue))
{
	Init(text, truncate);
}



template<class T>
Dialog::Dialog(T *t, void (T::*fun)(const std::string &), const std::string &text,
	std::string initialValue, Truncate truncate, bool allowsFastForward)
	: stringFun(std::bind(fun, t, std::placeholders::_1)),
	allowsFastForward(allowsFastForward),
	input(initialValue)
{
	Init(text, truncate);
}



template<class T>
Dialog::Dialog(T *t, void (T::*fun)(const std::string &), const std::string &text,
	std::function<bool(const std::string &)> validate, std::string initialValue, Truncate truncate, bool allowsFastForward)
	: stringFun(std::bind(fun, t, std::placeholders::_1)),
	validateFun(std::move(validate)),
	isOkDisabled(true),
	allowsFastForward(allowsFastForward),
	input(initialValue)
{
	Init(text, truncate);
}



template<class T>
Dialog::Dialog(T *t, void (T::*fun)(), const std::string &text, Truncate truncate, bool allowsFastForward)
	: voidFun(std::bind(fun, t)), allowsFastForward(allowsFastForward)
{
	Init(text, truncate);
}



template<class T>
Dialog::Dialog(T *t, void (T::*fun)(bool), const std::string &text, Truncate truncate, bool allowsFastForward)
	: boolFun(std::bind(fun, t, std::placeholders::_1)), allowsFastForward(allowsFastForward)
{
	Init(text, truncate);
}



template<class T>
Dialog::Dialog(T *panel,
	const std::string &text,
	const std::string &initialValue,
	Dialog::FunctionButton buttonOne,
	Dialog::FunctionButton buttonThree,
	std::function<bool(const std::string &)> validate)
	:
	validateFun(std::move(validate)),
	canCancel(true),
	input(initialValue),
	buttonOne(buttonOne),
	buttonThree(buttonThree)
{
	Init(text, Truncate::NONE);
}
