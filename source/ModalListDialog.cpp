/* ModalListDialog.cpp
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

#include "ModalListDialog.h"

#include "audio/Audio.h"
#include "Color.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "System.h"
#include "text/Truncate.h"
#include "UI.h"

using namespace std;



namespace {
	// Only show tooltips if the mouse has hovered in one place for this amount of time.
	constexpr int HOVER_TIME = 60;
}



ModalListDialog::ModalListDialog()
{
	Audio::Pause();
	SetInterruptible(false);
	UI::PlaySound(UI::UISound::SOFT);
}



ModalListDialog::~ModalListDialog()
{
	Audio::Resume();
}



void ModalListDialog::UpdateList(std::vector<std::string> newOptions)
{
	options.assign(newOptions.begin(), newOptions.end());
	bool found = false;
	for(auto &it : options)
	{
		if(it == selectedOption)
		{
			found = true;
			break;
		}
	}
	if(!found)
		selectedOption = options.front();
}



void ModalListDialog::Draw()
{
	DrawBackdrop();

	const Font &font = FontSet::Get(14);

	// The hover count "decays" over time if not hovering over a selection.
	if(hoverCount)
		--hoverCount;
	string hoverText;

	Information info;
	info.SetString("modal list title", title);
	info.SetString("button one label", buttonOne.buttonLabel);
	info.SetString("button two label", buttonTwo.buttonLabel);
	info.SetCondition("button one active");
	info.SetCondition("button two active");
	if(buttonThree.buttonAction)
	{
		info.SetString("button three label", buttonThree.buttonLabel);
		info.SetCondition("has button three");
		info.SetCondition("button three active");
	}

	if(activeButton == 1)
		info.SetCondition("button one focus");
	else if(activeButton == 2)
		info.SetCondition("button two focus");
	else if(activeButton == 3)
		info.SetCondition("button three focus");

	// Draw the static components, labels and buttons.
	const Interface *loadPanel = GameData::Interfaces().Get("modal list dialog");
	loadPanel->Draw(info, this);

	selectionListBox = loadPanel->GetBox("selection list");
	const Point topLeft = selectionListBox.TopLeft();
	Point currentTopLeft = topLeft + Point(0, -scrollY);
	const double top = topLeft.Y();
	const double bottom = top + selectionListBox.Height();
	const double hTextPad = loadPanel->GetValue("selection list horizontal text pad");
	const double fadeOut = loadPanel->GetValue("selection list fade out");

	// Draw the list of available selections.
	for(const string &display : options)
	{
		const Point drawPoint = currentTopLeft;
		currentTopLeft += Point(0., 20.);

		if(drawPoint.Y() < top - fadeOut)
			continue;
		if(drawPoint.Y() > bottom - fadeOut)
			continue;

		Rectangle zone(drawPoint + Point(selectionListBox.Width() / 2., 10.), Point(selectionListBox.Width(), 20.));
		const Point textPoint(drawPoint.X() + hTextPad, zone.Center().Y() - font.Height() / 2);
		bool isHovering = hasHover && zone.Contains(hoverPoint);
		bool isHighlighted = (display == selectedOption || isHovering);
		if(isHovering)
		{
			hoverCount = min(HOVER_TIME, hoverCount + 2);
			if(hoverCount == HOVER_TIME)
				hoverText = hoverFun(display);
		}

		double alpha = min((drawPoint.Y() - (top - fadeOut)) * .1,
				(bottom - fadeOut - drawPoint.Y()) * .1);
		alpha = max(alpha, 0.);
		alpha = min(alpha, 1.);

		if(display == selectedOption)
			FillShader::Fill(zone, Color(.1 * alpha, 0.));

		const int textWidth = selectionListBox.Width() - 2. * hTextPad;
		font.Draw({display, {textWidth, Truncate::BACK}}, textPoint, Color((isHighlighted ? .7 : .5) * alpha, 0.));
	}

	if(!hoverText.empty())
	{
		const Point boxSize(font.Width(hoverText) + 20., 30.);
		FillShader::Fill(Rectangle::FromCorner(hoverPoint, boxSize), *GameData::Colors().Get("tooltip background"));
		font.Draw(hoverText, hoverPoint + Point(10., 10.), *GameData::Colors().Get("medium"));
	}
}



bool ModalListDialog::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NORMAL;

	bool isCloseRequest = key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)));

	// Handle mouse clicks that are mapped via interfaces.txt
	// Button order is depicted on screen as 3, 2, 1 and mapped to F1, F2, F3 so keyboard order matches screen order.
	if(key == SDLK_F3 || key == buttonOne.buttonKey)
	{
		activeButton = 1;
		key = SDLK_RETURN;
	}
	else if(key == SDLK_F2 || key == buttonTwo.buttonKey)
	{
		activeButton = 2;
		key = SDLK_RETURN;
	}
	else if(key == SDLK_F1 || key == buttonThree.buttonKey)
	{
		activeButton = 3;
		key = SDLK_RETURN;
	}

	if(key == SDLK_TAB)
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
	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
	{
		// Now that we know what button was selected, process the button press
		if(DoCallback())
			GetUI()->Pop(this);
	}
	else if(key == isCloseRequest)
	{
		// TODO: consider if this should be handled differently
		GetUI()->Pop(this);
	}
	else if((key == SDLK_DOWN || key == SDLK_UP) && !options.empty())
	{
		// up/down selection within the list
		auto it = options.begin();
		int index = 0;
		for( ; it != options.end(); ++it, ++index)
			if(*it == selectedOption)
				break;

		if(key == SDLK_DOWN)
		{
			const int lastVisibleIndex = (scrollY / 20.) + 13.;
			if(index >= lastVisibleIndex)
				scrollY += 20.;
			++it;
			if(it == options.end())
			{
				it = options.begin();
				scrollY = 0.;
			}
		}
		else
		{
			const int firstVisibleIndex = scrollY / 20.;
			if(index <= firstVisibleIndex)
				scrollY -= 20.;
			if(it == options.begin())
			{
				it = options.end();
				scrollY = max(0., 20. * options.size() - (selectionListBox.Height() + 20));
			}
			--it;
		}
		selectedOption = *it;
	}
	else
		return false;

	UI::PlaySound(sound);
	return true;
}



bool ModalListDialog::Click(int x, int y, MouseButton button, int clicks)
{
	// When the user clicks, clear the hovered state.
	hasHover = false;
	if(button != MouseButton::LEFT)
		return false;

	const Point click(x, y);

	if(selectionListBox.Contains(click))
	{
		int selected = (y + scrollY - selectionListBox.Top()) / 20;
		int i = 0;
		for(const auto &it : options)
			if(i++ == selected && selectedOption != it)
			{
				selectedOption = it;
				UI::PlaySound(UI::UISound::NORMAL);
			}
	}
	else
		return false;

	return true;
}



bool ModalListDialog::Hover(int x, int y)
{
	hasHover = true;
	hoverPoint = Point(x, y);
	// Tooltips should not pop up unless the mouse stays in one place for the
	// full hover time. Otherwise, every time the user scrubs the mouse over the
	// list, tooltips will appear after one second.
	if(hoverCount < HOVER_TIME)
		hoverCount = 0;

	return true;
}



bool ModalListDialog::Drag(double dx, double dy)
{
	scrollY = max(0., min(20. * options.size() - (selectionListBox.Height() + 20), scrollY - dy));
	return true;
}



bool ModalListDialog::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



void ModalListDialog::Init()
{
	activeButton = 1;
	numButtons = 1;
	if(buttonTwo.buttonAction)
	{
		numButtons++;
		if(buttonThree.buttonAction)
			numButtons++;
	}
}



bool ModalListDialog::DoCallback() const
{
	bool closeDialog = false;
	if(activeButton == 1)
		closeDialog = buttonOne.buttonAction(selectedOption);
	else if(activeButton == 2 && buttonTwo.buttonAction)
		closeDialog = buttonTwo.buttonAction(selectedOption);
	else if(activeButton == 3 && buttonThree.buttonAction)
		closeDialog = buttonThree.buttonAction(selectedOption);
	return closeDialog;
}

