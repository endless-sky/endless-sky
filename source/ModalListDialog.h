/* ModalListDialog.h
Copyright (c) 2025 by xobes

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
#include "Dialog.h"

#include <string>

class WrappedText;



// This panel is shown when you hail a ship or planet. It allows you to ask for
// assistance from friendly ships, to bribe hostile ships to leave you alone, or
// to bribe a planet to allow you to land there.
class ModalListDialog : public Panel {
public:
	ModalListDialog();

	template<class T>
	ModalListDialog(T *panel,
		const std::string &title,
		const std::vector<std::string> &options,
		const std::string &initialSelection,
		Dialog::FunctionButton buttonOne,
		Dialog::FunctionButton buttonTwo,
		Dialog::FunctionButton buttonThree,
		std::string (T::*hoverFun)(const std::string &) = nullptr
		);

	virtual ~ModalListDialog() override;

	void UpdateList(std::vector<std::string> newOptions);
	virtual void Draw() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;

private:
	void Init();
	bool DoCallback() const;


private:
	std::string title;
	std::vector<std::string> options;
	std::string selectedOption;

	Dialog::FunctionButton buttonOne;
	Dialog::FunctionButton buttonTwo;
	Dialog::FunctionButton buttonThree;

	// Returns the tool tip when hovering.
	std::function<std::string (const std::string &)> hoverFun;

	Rectangle selectionListBox;

	int activeButton;
	int numButtons;

	Point hoverPoint;
	int hoverCount = 0;
	bool hasHover = false;
	double scrollY = 0;
};



template<class T>
ModalListDialog::ModalListDialog(T *panel,
	const std::string &title,
	const std::vector<std::string> &options,
	const std::string &initialSelection,
	const Dialog::FunctionButton buttonOne,
	const Dialog::FunctionButton buttonTwo,
	const Dialog::FunctionButton buttonThree,
	std::string(T::*hoverFun)(const std::string &))
	:
	title(title),
	selectedOption(initialSelection),
	buttonOne(buttonOne),
	buttonTwo(buttonTwo),
	buttonThree(buttonThree),
	hoverFun(std::bind(hoverFun, panel, std::placeholders::_1))
{
	Init();
	UpdateList(options);
}
