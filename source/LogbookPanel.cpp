/* LogbookPanel.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "LogbookPanel.h"

#include "text/Alignment.h"
#include "Color.h"
#include "Date.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "text/Layout.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Screen.h"
#include "image/SpriteSet.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>

using namespace std;

namespace {
	const double SIDEBAR_WIDTH = 100.;
	const double TEXT_WIDTH = 400.;
	const double PAD = 10.;
	const double WIDTH = SIDEBAR_WIDTH + TEXT_WIDTH;
	const double LINE_HEIGHT = 25.;

	// The minimum distance in pixels between the selected month and the edge of the screen before the month gets centered
	const double MINIMUM_SELECTION_DISTANCE = LINE_HEIGHT * 3;

	const double GAP = 30.;
	const string MONTH[] = {
		"  January", "  February", "  March", "  April", "  May", "  June",
		"  July", "  August", "  September", "  October", "  November", "  December"};
}



LogbookPanel::LogbookPanel(PlayerInfo &player)
	: MapPanel(player, SHOW_GOVERNMENT)
{
	isSimplified = true;
	player.CheckStorylineProgress();
	SetInterruptible(false);
	CreateSections();
	if(!sections.empty())
		selection = AvailableSelections(false).back();
}



void LogbookPanel::Step()
{
	MapPanel::Step();
}



void LogbookPanel::Draw()
{
	MapPanel::Draw();
	DrawOrbits();
	DrawKey();
	DrawLogbook();
	FinishDrawing("is ports");
}



bool LogbookPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NORMAL;

	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI().Pop(this);
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		double direction = (key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN);
		Drag(0., (Screen::Height() - 100.) * direction);
		sound = UI::UISound::NONE;
	}
	else if(key == SDLK_HOME || key == SDLK_END)
	{
		double direction = (key == SDLK_HOME) - (key == SDLK_END);
		Drag(0., maxScroll * direction);
		sound = UI::UISound::NONE;
	}
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		// Find the index of the currently selected line.
		vector<pair<string, string>> selections = AvailableSelections(false);
		size_t i = 0;
		for( ; i < selections.size(); ++i)
			if(selections[i] == selection)
				break;
		if(i == selections.size())
			return true;

		if(key == SDLK_DOWN)
		{
			++i;
			// Skip expanded entries.
			if(selections[i].first.empty())
				++i;
			if(i >= selections.size())
				i = 0;
		}
		else if(i)
		{
			--i;
			// Skip expanded entries.
			if(selections[i].first.empty())
			{
				// If this is the very top of the list, don't move the selection
				// up. (That is, you can't select the year heading line.)
				if(i)
					--i;
				else
					++i;
			}
		}
		else
			i = selections.size() - 1;
		if(selections[i] != selection)
		{
			selection = selections[i];
			scroll = 0.;

			selections = AvailableSelections();
			// Find our currently selected item again
			for(i = 0 ; i < selections.size(); ++i)
				if(selections[i] == selection)
					break;

			if(i == selections.size())
				return true;

			// Check if it's too far down or up
			int position = i * LINE_HEIGHT - categoryScroll;

			// If it's out of bounds, recenter it
			if(position < MINIMUM_SELECTION_DISTANCE || position > (Screen::Height() - MINIMUM_SELECTION_DISTANCE))
				categoryScroll = position - (Screen::Height() / 2);

			categoryScroll = max(categoryScroll, 0.);
		}
	}
	else
		return MapPanel::KeyDown(key, mod, command, isNewPress);

	UI::PlaySound(sound);
	return true;
}



bool LogbookPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;

	double sx = x - Screen::Left();
	double sy = y - Screen::Top();
	if(sx < SIDEBAR_WIDTH)
	{
		size_t index = (sy - PAD + categoryScroll) / LINE_HEIGHT;
		vector<pair<string, string>> selections = AvailableSelections();
		if(index < selections.size())
		{
			selection = selections[index];
			scroll = 0.;
			UI::PlaySound(UI::UISound::NORMAL);
		}
	}
	else if(sx > WIDTH)
		return MapPanel::Click(x, y, button, clicks);

	return true;
}



bool LogbookPanel::Drag(double dx, double dy)
{
	double sx = hoverPoint.X() - Screen::Left();
	if(sx > WIDTH)
		return MapPanel::Drag(dx, dy);
	if(sx > SIDEBAR_WIDTH)
		scroll = max(0., min(maxScroll, scroll - dy));
	else
		categoryScroll = max(0., min(maxCategoryScroll, categoryScroll - dy));

	return true;
}



bool LogbookPanel::Scroll(double dx, double dy)
{
	if(hoverPoint.X() - Screen::Left() > WIDTH)
		return MapPanel::Scroll(dx, dy);
	return Drag(0., dy * Preferences::ScrollSpeed());
}



bool LogbookPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	return MapPanel::Hover(x, y);
}



void LogbookPanel::CreateSections()
{
	sections.clear();
	// Special logs aren't expandable, and so they have the same category and subcategory name.
	for(const auto &[category, entries] : player.SpecialLogs())
	{
		Section &section = sections[category];
		if(!section.contains(category))
			section.insert({category, Page(PageType::SPECIAL)});
		Page &page = section.at(category);
		for(const auto &[heading, body] : entries)
			page.entries.emplace_back(EntryType::NORMAL, heading, body);
	}
	// For the rest of the logbook, the category is the year, the subcategory is the month.
	for(const auto &[date, entry] : player.Logbook())
	{
		string subcategory = MONTH[date.Month() - 1];
		Section &section = sections[to_string(date.Year())];
		if(!section.contains(subcategory))
			section.insert({subcategory, Page(PageType::DATE)});
		Page &page = section.at(subcategory);
		page.entries.emplace_back(EntryType::NORMAL, date.ToString(), entry);
	}
}



void LogbookPanel::DrawLogbook() const
{
	// Draw the panel. The sidebar should be slightly darker than the rest.
	const Color &sideColor = *GameData::Colors().Get("logbook sidebar");
	FillShader::Fill(
		Point(Screen::Left() + .5 * SIDEBAR_WIDTH, 0.),
		Point(SIDEBAR_WIDTH, Screen::Height()),
		sideColor);
	const Color &backColor = *GameData::Colors().Get("logbook background");
	FillShader::Fill(
		Point(Screen::Left() + SIDEBAR_WIDTH + .5 * TEXT_WIDTH, 0.),
		Point(TEXT_WIDTH, Screen::Height()),
		backColor);
	const Color &lineColor = *GameData::Colors().Get("logbook line");
	FillShader::Fill(
		Point(Screen::Left() + SIDEBAR_WIDTH - .5, 0.),
		Point(1., Screen::Height()),
		lineColor);

	Panel::DrawEdgeSprite(SpriteSet::Get("ui/right edge"), Screen::Left() + WIDTH);

	// Colors to be used for drawing the log.
	const Font &font = FontSet::Get(14);
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	// Draw the sidebar.
	// The currently selected sidebar item should be highlighted. This is how
	// big the highlight rectangle is.
	Point highlightSize(SIDEBAR_WIDTH - 4., LINE_HEIGHT);
	Point highlightOffset = Point(4. - PAD, 0.) + .5 * highlightSize;
	Point textOffset(0., .5 * (LINE_HEIGHT - font.Height()));
	// Start at this point on the screen:
	Point pos = Screen::TopLeft() + Point(PAD, PAD - categoryScroll);
	for(const auto &[name, section] : sections)
	{
		if(selection.first == name && selection.first == selection.second)
		{
			FillShader::Fill(pos + highlightOffset - Point(1., 0.), highlightSize + Point(0., 2.), lineColor);
			FillShader::Fill(pos + highlightOffset, highlightSize, backColor);
		}
		font.Draw(name, pos + textOffset, bright);
		pos.Y() += LINE_HEIGHT;
		if(selection.first != name || selection.first == selection.second)
			continue;

		for(const auto &[name, page] : section)
		{
			if(selection.second == name)
			{
				FillShader::Fill(pos + highlightOffset - Point(1., 0.), highlightSize + Point(0., 2.), lineColor);
				FillShader::Fill(pos + highlightOffset, highlightSize, backColor);
			}
			font.Draw(name, pos + textOffset, medium);
			pos.Y() += LINE_HEIGHT;
		}
	}

	maxCategoryScroll = max(0., maxCategoryScroll + pos.Y() - Screen::Bottom());

	auto it = sections.find(selection.first);
	if(it == sections.end())
		return;
	const Section &section = it->second;
	auto sit = section.find(selection.second);
	if(sit == section.end())
		return;
	const Page &page = sit->second;

	// Parameters for drawing the main text:
	WrappedText wrap(font);
	wrap.SetAlignment(Alignment::JUSTIFIED);
	wrap.SetWrapWidth(TEXT_WIDTH - 2. * PAD);

	// Draw the main text.
	pos = Screen::TopLeft() + Point(SIDEBAR_WIDTH + PAD, PAD + .5 * (LINE_HEIGHT - font.Height()) - scroll);

	const auto layout = Layout(static_cast<int>(TEXT_WIDTH - 2. * PAD), Alignment::RIGHT);
	for(const Entry &entry : page.entries)
	{
		if(page.type == PageType::SPECIAL)
			font.Draw(entry.heading, pos + textOffset, bright);
		else
			font.Draw({entry.heading, layout}, pos + Point(0., textOffset.Y()), dim);
		pos.Y() += LINE_HEIGHT;

		pos.Y() += entry.body.Draw(pos, wrap, medium);
		pos.Y() += GAP;
	}

	maxScroll = max(0., scroll + pos.Y() - Screen::Bottom());
}



vector<pair<string, string>> LogbookPanel::AvailableSelections(bool visibleOnly) const
{
	vector<pair<string, string>> selections;
	for(const auto &[name, section] : sections)
	{
		if(section.size() == 1 && section.front().first == name)
			selections.emplace_back(name, name);
		else if(!visibleOnly || selection.first == name)
		{
			selections.emplace_back(make_pair("", ""));
			for(const auto &[pageName, page] : section)
				selections.emplace_back(name, pageName);
		}
		else
			selections.emplace_back(name, section.back().first);
	}
	return selections;
}
