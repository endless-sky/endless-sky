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



// A dialog box displays a given message to the player. The box will expand to
// fit the message, and may also include a text input field. The box may have
// only an "ok" button, or may also have a "cancel" button. If this dialog is
// introducing a mission, the buttons are instead "accept" and "decline". A
// callback function can be given to receive the player's response.
// There can be up to three buttons. They will appear right-to-left.
// Button 1 = OK / Accept
// Button 2 = Cancel / Decline
// Button 3 = Infrequently used, e.g.
// [Random ] [Cancel] [ OK ]
// [Discard] [Cancel] [ OK ]
//
// Dialogs can also accept text input:
// Text
// [input field                   ]
// [Button 3] [Button 2] [Button 1]
class Dialog : public Panel {
public:
	class FunctionButton {
	public:
		FunctionButton() = default;
		~FunctionButton() = default;

		template<class T>
		FunctionButton(T *panel, const std::string &buttonLabel, SDL_Keycode buttonKey = '\0',
			bool (T::*buttonAction)(const std::string&) = nullptr);

	public:
		std::string buttonLabel;
		SDL_Keycode buttonKey;
		std::function<bool(const std::string &)> buttonAction;
	};


public:
	// An OK / Cancel dialog where Cancel can be disabled. The activeButton lets
	// you select whether "OK" (1) or "Cancel" (2) are selected as the default option.
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

	// Three button context. Must provide actions for button 1 and button 3. Button 2 is cancel.
	template<class T>
	Dialog(T *panel, const std::string &text, const std::string &initialValue,
		Dialog::FunctionButton buttonOne, Dialog::FunctionButton buttonThree,
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


private:
	// Common code from all three constructors:
	void Init(const std::string &message, Truncate truncate, bool canCancel = true, bool isMission = false);
	void DoCallback(bool isOk = true) const;
	// The width of the dialog, excluding margins.
	int Width() const;


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
Dialog::FunctionButton::FunctionButton(T *panel, const std::string &buttonLabel, SDL_Keycode buttonKey,
	bool(T::*buttonAction)(const std::string &))
	: buttonLabel(buttonLabel), buttonKey(buttonKey),
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
Dialog::Dialog(T *panel, const std::string &text, const std::string &initialValue,
	Dialog::FunctionButton buttonOne, Dialog::FunctionButton buttonThree,
	std::function<bool(const std::string &)> validate)
	: validateFun(std::move(validate)), canCancel(true), input(initialValue),
	buttonOne(buttonOne), buttonThree(buttonThree)
{
	Init(text, Truncate::NONE);
}
