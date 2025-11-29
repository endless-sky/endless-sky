/* ControlsListDialog.cpp
Copyright (c) 2024 by xobes

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ControlsListDialog.h"

#include "Dialog.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Logger.h"
#include "Preferences.h"
#include "Screen.h"
#include "TextArea.h"
#include "image/SpriteSet.h"
#include "UI.h"
#include "image/Sprite.h"

using namespace std;
using namespace dialog;

namespace {
	// Only show tooltips if the mouse has hovered in one place for this amount of time.
	constexpr int HOVER_TIME = 60;
}



void ControlsListDialog::UpdateList(std::vector<std::string> newOptions)
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



void ControlsListDialog::Draw()
{
	Dialog::Draw();

	const Font &font = FontSet::Get(14);
	const Color bright = *GameData::Colors().Get("bright");

	const Point topLeft = selectionListBox.TopLeft();

	// Draw title with underline
	font.Draw(title, {topLeft.X(), topLeft.Y() - 30}, bright);
	FillShader().Fill(Point(0, topLeft.Y() - TOP_PADDING), {Width() - HORIZONTAL_PADDING, 1}, bright);

	const double top = topLeft.Y();
	const double bottom = selectionListBox.Bottom();
	const double hTextPad = 5;
	const double fadeOut = 10;

	// The hover count "decays" over time if not hovering over a selection.
	if(hoverCount)
		--hoverCount;
	string hoverText;

	Point currentTopLeft = topLeft + Point(0, -scrollY);

	// Draw the list of available selections.
	for(const string &display : options)
	{
		const Point drawPoint = currentTopLeft;
		currentTopLeft += Point(0., 20.);

		if(drawPoint.Y() < top - 20)
			continue;
		if(drawPoint.Y() > bottom)
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

		double alpha = 1;
		// double alpha = min((drawPoint.Y() - (top - fadeOut)) * .1,
		// 		(bottom - fadeOut - drawPoint.Y()) * .1);
		// alpha = max(alpha, 0.);
		// alpha = min(alpha, 1.);

		if(display == selectedOption)
			FillShader::Fill(zone, Color(.1 * alpha, 0.));

		const int textWidth = selectionListBox.Width() - 2. * hTextPad;
		font.Draw(DisplayText(display, {textWidth, Truncate::BACK}), textPoint, Color((isHighlighted ? .7 : .5) * alpha, 0.));
	}
	// Draw the top and bottom fader graphics
	Color background = *GameData::Colors().Get("map side panel background");
	Point drawTop = Point(0, selectionListBox.Top() - fadeOut);
	Point drawBottom = Point(0, selectionListBox.Bottom() + fadeOut);
	for(int i = 0; i < fadeOut; i++) {
		double alpha = min(max((fadeOut - i) * .1, 0.), 1.);
		FillShader().Fill(drawTop + Point(0, i), {selectionListBox.Width(), 1}, background.Transparent(alpha));
		FillShader().Fill(drawBottom - Point(0, i), {selectionListBox.Width(), 1}, background.Transparent(alpha));
	}

	if(!hoverText.empty())
	{
		const Point boxSize(font.Width(hoverText) + 20., 30.);
		FillShader::Fill(Rectangle::FromCorner(hoverPoint, boxSize), *GameData::Colors().Get("tooltip background"));
		font.Draw(hoverText, hoverPoint + Point(10., 10.), *GameData::Colors().Get("medium"));
	}
}



bool ControlsListDialog::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NORMAL;

	bool isCloseRequest = key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)));

	if(key == buttonOne.buttonKey)
	{
		activeButton = 1;
		key = SDLK_RETURN;
	}
	else if(isCloseRequest)
	{
		return true;
	}
	else if(key == buttonThree.buttonKey)
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

		int firstVisibleIndex = max(0., scrollY + 10) / 20.;
		int lastVisibleIndex = firstVisibleIndex + (selectionListBox.Height() / 20) - 1;

		if(key == SDLK_DOWN)
		{
			if(index >= lastVisibleIndex)
				scrollY += 20.;
			++it;
			++index;
			if(it == options.end())
			{
				it = options.begin();
				scrollY = 0.;
				index = 0;
			}
		}
		else
		{
			if(index <= firstVisibleIndex)
				scrollY -= 20.;
			if(it == options.begin())
			{
				it = options.end();
				scrollY = max(0., 20. * options.size() - selectionListBox.Height());
				index = options.size();
			}
			--it;
			--index;
		}
		selectedOption = *it;

		// Adjust scroll to ensure that after an up/down keyboard adjustment the active option is visible (Next update).
		firstVisibleIndex = max(0., scrollY + 10) / 20.;
		lastVisibleIndex = firstVisibleIndex + (selectionListBox.Height() / 20) - 1;
		scrollY += max(0, index - lastVisibleIndex) * 20 - max(0, firstVisibleIndex - index) * 20;
		scrollY = max(0., min(scrollY, 20 * options.size() - selectionListBox.Height()));
	}
	else
		return false;

	UI::PlaySound(sound);
	return true;
}



bool ControlsListDialog::Click(int x, int y, MouseButton button, int clicks)
{

	// When the user clicks, clear the hovered state.
	hasHover = false;
	if(button != MouseButton::LEFT)
		return false;

	const Point clickPos(x, y);

	if(selectionListBox.Contains(clickPos))
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
		return Dialog::Click(x, y, button, clicks);

	return true;
}



void ControlsListDialog::Resize()
{
	Dialog::Resize(height);
	Rectangle rect = text->GetRect();
	selectionListBox = Rectangle::FromCorner(rect.TopLeft() + Point(0, 30), rect.Dimensions() - Point(0, 32));
	// Move the text area out of the way so it doesn't steal clicks and scrolling.
	text->SetRect(Rectangle::FromCorner(Screen::BottomRight(), {0, 0}));
}



bool ControlsListDialog::Hover(int x, int y)
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



bool ControlsListDialog::Drag(double dx, double dy)
{
	scrollY = max(0., min(20. * options.size() - (selectionListBox.Height()), scrollY - dy));
	return true;
}



bool ControlsListDialog::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



bool ControlsListDialog::DoCallback() const
{
	bool closeDialog = false;
	if(activeButton == 1)
		closeDialog = buttonOne.buttonAction(selectedOption);
	else if(activeButton == 2)
		closeDialog = true;
	else if(activeButton == 3 && buttonThree.buttonAction)
		closeDialog = buttonThree.buttonAction(selectedOption);
	return closeDialog;
}
