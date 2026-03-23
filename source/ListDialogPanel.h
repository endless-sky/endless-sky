/* ListDialogPanel.h
Copyright (c) 2026 by xobes

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

#include "DialogPanel.h"

#include "ClickZone.h"
#include "GameData.h"
#include "ScrollVar.h"
#include "Tooltip.h"

#include <string>

class RenderBuffer;


class ListDialogInit {
public:
	std::string title;
	std::vector<std::string> options;
	std::function<std::string (const std::string &)> hoverFun;
	std::string selectedItem;
	// Note: tooltip must be fully initialized as it has no default constructor.
	Tooltip tooltip;
};


// A special version of Dialog for listing the command profiles.
class ListDialogPanel : public DialogPanel {
public:
	template<class T>
	static ListDialogPanel *ShowList(T *t, const std::string &title,
		const std::vector<std::string> &options, const std::string &initialSelection,
		DialogPanel::FunctionButton buttonOne, DialogPanel::FunctionButton buttonThree,
		std::string (T::*hoverFun)(const std::string&));

	void UpdateList(std::vector<std::string> newOptions);

	bool AcceptsInput() const;

	virtual void Draw() override;

	explicit ListDialogPanel(DialogInit &init, ListDialogInit &init2);


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	// virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual void Resize() override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	void ScrollToSelection();
	void DrawTooltips();


private:
	bool DoCallback() const;


private:
	std::string title;
	std::vector<std::string> options;

	// Height will resolve to a number of extensionCount middle panels and is not exact.
	int height = 100;

	Rectangle selectionListBox;

	int selectedIndex = 0;
	std::string selectedItem;
	std::string hoverItem;
	std::function<std::string (const std::string &)> hoverFun;

	Point hoverPoint;
	Tooltip tooltip;

	std::vector<ClickZone<std::string>> optionZones;
	std::unique_ptr<RenderBuffer> listClip;
	ScrollVar<double> listScroll;
};



template<class T>
ListDialogPanel *ListDialogPanel::ShowList(
	T *t,
	const std::string &title,
	const std::vector<std::string> &options,
	const std::string &initialSelection,
	DialogPanel::FunctionButton buttonOne,
	DialogPanel::FunctionButton buttonThree,
	std::string(T::*hoverFun)(const std::string &)
	)
{
	DialogInit init;
	init.buttonOne = buttonOne;
	init.buttonThree = buttonThree;

	ListDialogInit init2 = {
		std::move(title),
		std::move(options),
		std::bind(hoverFun, t, std::placeholders::_1),
		std::move(initialSelection),
		{130, Alignment::CENTER, Tooltip::Direction::DOWN_LEFT, Tooltip::Corner::TOP_LEFT,
			GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium")}
	};

	return new ListDialogPanel(init, init2);
}
