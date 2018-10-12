/* LogbookPanel.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "LogbookPanel.h"

#include "Color.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"
#include "WrappedText.h"

#include <set>

using namespace std;

namespace {
	const double SIDEBAR_WIDTH = 100.;
	const double TEXT_WIDTH = 400.;
	const double PAD = 10.;
	const double WIDTH = SIDEBAR_WIDTH + TEXT_WIDTH;
	const double LINE_HEIGHT = 25.;
	const double GAP = 30.;
	const string MONTH[] = {
		"  January", "  February", "  March", "  April", "  May", "  June",
		"  July", "  August", "  September", "  October", "  November", "  December"};
}



LogbookPanel::LogbookPanel(PlayerInfo &player)
	: player(player)
{
	SetInterruptible(false);
	if(!player.Logbook().empty())
	{
		selectedDate = (--player.Logbook().end())->first;
		selectedName = MONTH[selectedDate.Month() - 1];
	}
	Update();
}



// Draw this panel.
void LogbookPanel::Draw()
{
	// Dim out everything outside this panel.
	DrawBackdrop();
	
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
	
	const Sprite *edgeSprite = SpriteSet::Get("ui/right edge");
	if(edgeSprite->Height())
	{
		// If the screen is high enough, the edge sprite should repeat.
		double spriteHeight = edgeSprite->Height();
		Point pos(
			Screen::Left() + WIDTH + .5 * edgeSprite->Width(),
			Screen::Top() + .5 * spriteHeight);
		for( ; pos.Y() - .5 * spriteHeight < Screen::Bottom(); pos.Y() += spriteHeight)
			SpriteShader::Draw(edgeSprite, pos);
	}
	
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
	Point pos = Screen::TopLeft() + Point(PAD, PAD);
	for(size_t i = 0; i < contents.size(); ++i)
	{
		if(selectedDate ? dates[i].Month() == selectedDate.Month() : selectedName == contents[i])
		{
			FillShader::Fill(pos + highlightOffset - Point(1., 0.), highlightSize + Point(0., 2.), lineColor);
			FillShader::Fill(pos + highlightOffset, highlightSize, backColor);
		}
		font.Draw(contents[i], pos + textOffset, dates[i].Month() ? medium : bright);
		pos.Y() += LINE_HEIGHT;
	}
	
	// Parameters for drawing the main text:
	WrappedText wrap(font);
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(TEXT_WIDTH - 2. * PAD);
	
	// Draw the main text.
	pos = Screen::TopLeft() + Point(SIDEBAR_WIDTH + PAD, PAD + .5 * (LINE_HEIGHT - font.Height()) - scroll);
	
	// Branch based on whether this is an ordinary log month or a special page.
	auto pit = player.SpecialLogs().find(selectedName);
	if(selectedDate && begin != end)
	{
		for(auto it = begin; it != end; ++it)
		{
			string date = it->first.ToString();
			double dateWidth = font.Width(date);
			font.Draw(date, pos + Point(TEXT_WIDTH - dateWidth - 2. * PAD, textOffset.Y()), dim);
			pos.Y() += LINE_HEIGHT;
		
			wrap.Wrap(it->second);
			wrap.Draw(pos, medium);
			pos.Y() += wrap.Height() + GAP;
		}
	}
	else if(!selectedDate && pit != player.SpecialLogs().end())
	{
		for(const auto &it : pit->second)
		{
			font.Draw(it.first, pos + textOffset, bright);
			pos.Y() += LINE_HEIGHT;
		
			wrap.Wrap(it.second);
			wrap.Draw(pos, medium);
			pos.Y() += wrap.Height() + GAP;
		}
	}
	
	maxScroll = max(0., scroll + pos.Y() - Screen::Bottom());
}



bool LogbookPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		double direction = (key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN);
		Drag(0., (Screen::Height() - 100.) * direction);
	}
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		// Find the index of the currently selected line.
		size_t i = 0;
		for( ; i < contents.size(); ++i)
			if(contents[i] == selectedName)
				break;
		if(i == contents.size())
			return true;
		
		if(key == SDLK_DOWN)
		{
			++i;
			if(i >= contents.size())
				i = 0;
		}
		else if(i)
		{
			--i;
			// Skip the entry that is just the currently selected year.
			if(dates[i] && !dates[i].Month())
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
			i = contents.size() - 1;
		if(contents[i] != selectedName)
		{
			selectedDate = dates[i];
			selectedName = contents[i];
			scroll = 0.;
			Update(key == SDLK_UP);
		}
	}
	
	return true;
}



bool LogbookPanel::Click(int x, int y, int clicks)
{
	x -= Screen::Left();
	y -= Screen::Top();
	if(x < SIDEBAR_WIDTH)
	{
		size_t index = (y - PAD) / LINE_HEIGHT;
		if(index < contents.size())
		{
			selectedDate = dates[index];
			selectedName = contents[index];
			scroll = 0.;
			// If selecting a different year, select the first month in that
			// year.
			Update(false);
		}
	}
	else if(x > WIDTH)
		GetUI()->Pop(this);
	
	return true;
}



bool LogbookPanel::Drag(double dx, double dy)
{
	scroll = max(0., min(maxScroll, scroll - dy));
	
	return true;
}



bool LogbookPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



void LogbookPanel::Update(bool selectLast)
{
	contents.clear();
	dates.clear();
	for(const auto &it : player.SpecialLogs())
	{
		contents.emplace_back(it.first);
		dates.emplace_back();
	}
	// The logbook should never be opened if it has no entries, but just in case:
	if(player.Logbook().empty())
	{
		begin = end = player.Logbook().end();
		return;
	}
	
	// Check what years and months have entries for them.
	set<int> years;
	set<int> months;
	for(const auto &it : player.Logbook())
	{
		years.insert(it.first.Year());
		if(it.first.Year() == selectedDate.Year() && it.first.Month() >= 1 && it.first.Month() <= 12)
			months.insert(it.first.Month());
	}
	
	// Generate the table of contents.
	for(int year : years)
	{
		contents.emplace_back(to_string(year));
		dates.emplace_back(0, 0, year);
		if(selectedDate && year == selectedDate.Year())
			for(int month : months)
			{
				contents.emplace_back(MONTH[month - 1]);
				dates.emplace_back(0, month, year);
			}
	}
	// If a special category is selected, bail out here.
	if(!selectedDate)
	{
		begin = end = player.Logbook().end();
		return;
	}
	
	// Make sure a month is selected, within the current year.
	if(!selectedDate.Month())
	{
		selectedDate = Date(0, selectLast ? *--months.end() : *months.begin(), selectedDate.Year());
		selectedName = MONTH[selectedDate.Month() - 1];
	}
	// Get the range of entries that include the selected month.
	begin = player.Logbook().lower_bound(Date(0, selectedDate.Month(), selectedDate.Year()));
	end = player.Logbook().lower_bound(Date(32, selectedDate.Month(), selectedDate.Year()));
}
