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
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "RenderBuffer.h"
#include "Screen.h"
#include "text/Table.h"
#include "TextArea.h"
#include "UI.h"

using namespace std;
using namespace dialog;



void ControlsListDialog::UpdateList(std::vector<std::string> newOptions) {
	options.assign(newOptions.begin(), newOptions.end());
	bool found = false;
	int index = 0;
	for(auto &it : options)
	{
		if(it == selectedItem)
		{
			selectedIndex = index;
			found = true;
			break;
		}
		++index;
	}
	if(!found)
	{
		selectedItem = options.front();
		selectedIndex = 0;
	}

	// Set the new list scroll range.
	listScroll.SetMaxValue(20 * options.size());

	ScrollToSelection();
}



void ControlsListDialog::Draw()
{
	ClearZones();
	optionZones.clear();

	Dialog::Draw();

	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &faint = *GameData::Colors().Get("faint");

	const Point topLeft = selectionListBox.TopLeft();

	// Draw title with underline
	font.Draw(title, {topLeft.X(), topLeft.Y() - 30}, bright);
	FillShader().Fill(Point(0, topLeft.Y() - TOP_PADDING), {Width() - HORIZONTAL_PADDING, 1}, bright);

	// Animate scrolling.
	listScroll.Step();

	// Switch render target to listClip. Until target is destroyed or
	// deactivated, all opengl commands will be drawn there instead.
	auto target = listClip->SetTarget();

	// Begin local coordinates.
	// Create a table, leave room for the scroll bar on the right.
	Table table;
	table.AddColumn(listClip->Left(), Layout(selectionListBox.Width() - 7, Truncate::MIDDLE));
	table.SetUnderline(listClip->Left(), listClip->Right() - 7);

	int firstY = listClip->Top();
	table.DrawAt(Point(0, firstY - static_cast<int>(listScroll.AnimatedValue())));

	int index = 0;
	for(const string &display : options)
	{
		// Add selectionListBox.Center() for absolute coordinates.
		optionZones.emplace_back(selectionListBox.Center() + table.GetCenterPoint(), table.GetRowSize(), display);

		bool isSelectedIndex = (display == selectedItem);
		if(isSelectedIndex || display == hoverItem)
			table.DrawHighlight(faint);

		Rectangle zoneBounds = table.GetRowBounds();

		// Only include the zone as clickable if it's within the drawing area.
		bool displayed = table.GetPoint().Y() > listClip->Top() - 20 &&
			table.GetPoint().Y() < listClip->Bottom() - table.GetRowBounds().Height() + 20;
		if(displayed)
			// Add selectionListBox.Center() for absolute coordinates.
			AddZone({selectionListBox.Center() + zoneBounds.Center(), zoneBounds.Dimensions()},
					[&]() { selectedItem = display; selectedIndex = index; });
		table.Draw(display, isSelectedIndex ? bright : medium);

		++index;
	}

	// Switch back to normal opengl operations.
	target.Deactivate();

	listClip->SetFadePadding(
		listScroll.IsScrollAtMin() ? 0 : 20,
		listScroll.IsScrollAtMax() ? 0 : 20
	);

	// Draw the scrolled and clipped plugin list to the screen.
	listClip->Draw(selectionListBox.Center());
	const Point UP{0, -1};
	const Point DOWN{0, 1};
	const Point POINTER_OFFSET{0, 5};
	if(listScroll.Scrollable())
	{
		// Draw up and down pointers, mostly to indicate when scrolling
		// is possible, but might as well make them clickable too.
		Rectangle topRight({selectionListBox.Right(), selectionListBox.Top() + POINTER_OFFSET.Y()}, {20.0, 20.0});
		PointerShader::Draw(topRight.Center(), UP,
			10.f, 10.f, 5.f, Color(listScroll.IsScrollAtMin() ? .2f : .8f, 0.f));
		AddZone(topRight, [&]() { listScroll.Scroll(-Preferences::ScrollSpeed()); });

		Rectangle bottomRight(selectionListBox.BottomRight() - POINTER_OFFSET, {20.0, 20.0});
		PointerShader::Draw(bottomRight.Center(), DOWN,
			10.f, 10.f, 5.f, Color(listScroll.IsScrollAtMax() ? .2f : .8f, 0.f));
		AddZone(bottomRight, [&]() { listScroll.Scroll(Preferences::ScrollSpeed()); });
	}

	DrawTooltips();
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
		GetUI()->Pop(this);
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
		// Now that we know what button was selectedIndex, process the button press
		if(DoCallback())
			GetUI()->Pop(this);
	}
	else if(key == isCloseRequest)
	{
		GetUI()->Pop(this);
	}
	else if((key == SDLK_DOWN || key == SDLK_UP) && !options.empty())
	{
		if(key == SDLK_DOWN)
		{
			++selectedIndex;
			if(static_cast<unsigned>(selectedIndex) >= options.size())
				selectedIndex = 0;
		}
		else
		{
			--selectedIndex;
			if(selectedIndex < 0)
				selectedIndex = options.size() - 1;
		}

		auto it = options.begin();
		int index = 0;
		for( ; it != options.end(); ++it, ++index)
			if(index == selectedIndex)
			{
				selectedItem = *it;
				break;
			}

		ScrollToSelection();
	}
	else
		return false;

	UI::PlaySound(sound);
	return true;
}



void ControlsListDialog::Resize()
{
	Dialog::Resize(height);
	selectionListBox = Rectangle::FromCorner(textRect.TopLeft() + Point(0, 30), textRect.Dimensions() - Point(0, 32));
	listScroll.SetDisplaySize(selectionListBox.Height());
	listClip = make_unique<RenderBuffer>(selectionListBox.Dimensions());

	// Move the TextArea out of the way so it doesn't steal clicks and scroll actions. We are not using it.
	text->SetRect(Rectangle::FromCorner(Screen::BottomRight(), {0, 0}));
}



bool ControlsListDialog::Hover(int x, int y)
{
	hoverPoint = Point(x, y);

	hoverItem.clear();
	tooltip.Clear();

	for(const auto &zone : optionZones)
		if(zone.Contains(hoverPoint))
		{
			hoverItem = zone.Value();
			tooltip.SetZone(zone);
		}

	return true;
}



bool ControlsListDialog::Drag(double dx, double dy)
{
	// Steps is zero so that we don't animate mouse drags.
	listScroll.Scroll(-dy, 0);
	return true;
}



bool ControlsListDialog::Scroll(double dx, double dy)
{
	listScroll.Scroll(-dy * Preferences::ScrollSpeed());
	return true;
}



void ControlsListDialog::ScrollToSelection()
{
	while(selectedIndex * 20 - listScroll < 0)
		listScroll.Scroll(-Preferences::ScrollSpeed());
	while((selectedIndex + 1) * 20 - listScroll > listClip->Height())
		listScroll.Scroll(Preferences::ScrollSpeed());
}



bool ControlsListDialog::DoCallback() const
{
	bool closeDialog = false;
	if(activeButton == 1)
		closeDialog = buttonOne.buttonAction(selectedItem);
	else if(activeButton == 2)
		closeDialog = true;
	else if(activeButton == 3 && buttonThree.buttonAction)
		closeDialog = buttonThree.buttonAction(selectedItem);
	return closeDialog;
}



void ControlsListDialog::DrawTooltips()
{
	if(hoverItem.empty())
	{
		tooltip.DecrementCount();
		return;
	}
	tooltip.IncrementCount();
	if(!tooltip.ShouldDraw())
		return;

	tooltip.SetText(hoverFun(hoverItem));

	if(!tooltip.HasText())
		tooltip.SetText(GameData::Tooltip(hoverItem));

	tooltip.Draw();
}
