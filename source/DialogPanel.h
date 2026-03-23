/* DialogPanel.h
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

#include "text/Format.h"
#include "Point.h"
#include "text/Truncate.h"

#include <functional>
#include <optional>
#include <string>

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
class DialogPanel : public Panel {
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
		SDL_Keycode buttonKey{};
		std::function<bool(const std::string &)> buttonAction;
	};


public:
	virtual ~DialogPanel() override;

	// An OK dialog that has no callback or cancel button. Only used for displaying information.
	static DialogPanel *Info(std::string message, Truncate truncate = Truncate::NONE, bool allowsFastForward = false);

	// OK / Cancel dialog.
	// The callback is always called with the value of what button the user clicked (ok == true, cancel == false).
	template<class T>
	static DialogPanel *CallFunctionOnExit(T *t, void (T::*fun)(bool),
		std::string message,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);
	// OK / Cancel dialogs.
	// If the user selects "ok", the callback is called with no parameters.
	template<class T>
	static DialogPanel *CallFunctionIfOk(T *t, void (T::*fun)(),
		std::string message,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);
	static DialogPanel *CallFunctionIfOk(std::function<void()> okFunction,
		std::string message,
		int activeButton,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);

	// Accept / Decline dialog for missions.
	// Calls PlayerInfo::MissionCallback with the respective Endpoint of the selected button as input.
	static DialogPanel *MissionOfferDialog(std::string message,
		PlayerInfo &player,
		const System *system = nullptr,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);

	// OK / Cancel dialogs that request input of differing types.
	// If the user selects "ok", the callback is called with the input.
	template<class T>
	static DialogPanel *RequestString(T *t, void (T::*fun)(const std::string &),
		std::string message,
		std::string initialValue = "",
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);
	template<class T>
	static DialogPanel *RequestInteger(T *t, void (T::*fun)(int),
		std::string message,
		std::optional<int> initialValue = std::nullopt,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);
	template<class T>
	static DialogPanel *RequestDouble(T *t, void (T::*fun)(double),
		std::string message,
		std::optional<double> initialValue = std::nullopt,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);

	// OK / Cancel dialogs that request input but with validation.
	// The "ok" button is disabled if the validation callback returns false.
	// If the user selects "ok", the callback is called with the input.
	template<class T>
	static DialogPanel *RequestStringWithValidation(T *t, void (T::*fun)(const std::string &),
		std::function<bool(const std::string &)> validate,
		std::string message,
		std::string initialValue = "",
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);
	template<class T>
	static DialogPanel *RequestIntegerWithValidation(T *t, void (T::*fun)(int),
		std::function<bool(int)> validate,
		std::string message,
		std::optional<int> initialValue = std::nullopt,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);
	template<class T>
	static DialogPanel *RequestDoubleWithValidation(T *t, void (T::*fun)(double),
		std::function<bool(double)> validate,
		std::string message,
		std::optional<double> initialValue = std::nullopt,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);

	// An OK / Cancel dialog that requests that the user inputs an integer that is greater than zero.
	template<class T>
	static DialogPanel *RequestPositiveInteger(T *t, void (T::*fun)(int),
		std::string message,
		std::optional<int> initialValue = std::nullopt,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);

	// Draw this panel.
	virtual void Draw() override;

	// Some dialogs allow fast-forward to stay active.
	bool AllowsFastForward() const noexcept final;


protected:
	class DialogInit {
	public:
		std::string message;
		std::string initialValue;
		Truncate truncate = Truncate::NONE;

		std::function<void()> voidFun;
		std::function<void(bool)> boolFun;
		std::function<void(int)> intFun;
		std::function<void(double)> doubleFun;
		std::function<void(const std::string &)> stringFun;

		std::function<bool(int)> validateIntFun;
		std::function<bool(double)> validateDoubleFun;
		std::function<bool(const std::string &)> validateStringFun;

		bool canCancel = true;
		int activeButton = 1;
		bool isMission = false;
		bool allowsFastForward = false;

		DialogPanel::FunctionButton buttonOne;
		DialogPanel::FunctionButton buttonThree;

		const System *system = nullptr;
		PlayerInfo *player = nullptr;
	};


protected:
	explicit DialogPanel(DialogInit &init);

	virtual void Resize() override;

	// The user can click "ok" or "cancel", or use the tab key to toggle which
	// button is highlighted and the enter key to select it.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;


private:
	void DoCallback(bool isOk = true) const;
	// The width of the dialog, excluding margins.
	int Width() const;
	// Whether this dialog accepts typed input from the player.
	bool AcceptsInput() const;
	// Return true if the validation function passes when given the current input,
	// or if there is no validation function.
	bool ValidateInput() const;


protected:
	std::shared_ptr<TextArea> text;
	// The number of extra segments in this dialog.
	int extensionCount;

	std::function<void()> voidFun;
	std::function<void(bool)> boolFun;
	std::function<void(int)> intFun;
	std::function<void(double)> doubleFun;
	std::function<void(const std::string &)> stringFun;

	std::function<bool(int)> validateIntFun;
	std::function<bool(double)> validateDoubleFun;
	std::function<bool(const std::string &)> validateStringFun;

	bool canCancel;
	int activeButton;
	bool isMission;
	bool isOkDisabled;
	bool allowsFastForward;
	bool isWide;

	std::string input;

	std::string okText;
	std::string cancelText;

	Point okPos;
	Point cancelPos;
	Point thirdPos;

	DialogPanel::FunctionButton buttonOne;
	DialogPanel::FunctionButton buttonThree;

	int numButtons;

	const System *system = nullptr;
	PlayerInfo *player = nullptr;
};



template<class T>
DialogPanel::FunctionButton::FunctionButton(T *panel, const std::string &buttonLabel, SDL_Keycode buttonKey,
	bool(T::*buttonAction)(const std::string &))
	: buttonLabel(buttonLabel), buttonKey(buttonKey),
	buttonAction(std::bind(buttonAction, panel, std::placeholders::_1))
{
}



template<class T>
DialogPanel *DialogPanel::CallFunctionOnExit(T *t, void (T::*fun)(bool), std::string message,
	Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	init.boolFun = std::bind(fun, t, std::placeholders::_1);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::CallFunctionIfOk(T *t, void (T::*fun)(), std::string message,
	Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	init.voidFun = std::bind(fun, t);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::RequestString(T *t, void (T::*fun)(const std::string &), std::string message,
	std::string initialValue, Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	init.initialValue = std::move(initialValue);
	init.stringFun = std::bind(fun, t, std::placeholders::_1);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::RequestInteger(T *t, void (T::*fun)(int), std::string message,
	std::optional<int> initialValue, Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	if(initialValue.has_value())
		init.initialValue = std::to_string(initialValue.value());
	init.intFun = std::bind(fun, t, std::placeholders::_1);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::RequestDouble(T *t, void (T::*fun)(double), std::string message,
	std::optional<double> initialValue, Truncate truncate,
	bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	if(initialValue.has_value())
		init.initialValue = Format::StripCommas(Format::Number(initialValue.value(), 5));
	init.doubleFun = std::bind(fun, t, std::placeholders::_1);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::RequestStringWithValidation(T *t, void (T::*fun)(const std::string &),
	std::function<bool(const std::string &)> validate, std::string message, std::string initialValue,
	Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	init.initialValue = std::move(initialValue);
	init.stringFun = std::bind(fun, t, std::placeholders::_1);
	init.validateStringFun = std::move(validate);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::RequestIntegerWithValidation(T *t, void (T::*fun)(int),
	std::function<bool(int)> validate, std::string message, std::optional<int> initialValue,
	Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	if(initialValue.has_value())
		init.initialValue = std::to_string(initialValue.value());
	init.intFun = std::bind(fun, t, std::placeholders::_1);
	init.validateIntFun = std::move(validate);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::RequestDoubleWithValidation(T *t, void (T::*fun)(double),
	std::function<bool(double)> validate, std::string message, std::optional<double> initialValue,
	Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	if(initialValue.has_value())
		init.initialValue = Format::StripCommas(Format::Number(initialValue.value(), 5));
	init.doubleFun = std::bind(fun, t, std::placeholders::_1);
	init.validateDoubleFun = std::move(validate);
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new DialogPanel(init);
}



template<class T>
DialogPanel *DialogPanel::RequestPositiveInteger(T *t, void (T::*fun)(int), std::string message,
	std::optional<int> initialValue, Truncate truncate, bool allowsFastForward)
{
	return DialogPanel::RequestIntegerWithValidation(t, fun, [](int value) -> bool { return value > 0; },
		message, initialValue, truncate, allowsFastForward);
}
