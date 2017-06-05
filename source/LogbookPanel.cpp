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
}



LogbookPanel::LogbookPanel(PlayerInfo &player)
	: player(player)
{
	SetInterruptible(false);
	if(!player.Logbook().empty())
		selected = (--player.Logbook().end())->first;
	Update();
}



// Draw this panel.
void LogbookPanel::Draw()
{
	// Dim out everything outside this panel.
	DrawBackdrop();
	
	// Draw the panel. The sidebar should be slightly darker than the rest.
	Color sideColor(.09, 1.);
	FillShader::Fill(
		Point(Screen::Left() + .5 * SIDEBAR_WIDTH, 0.),
		Point(SIDEBAR_WIDTH, Screen::Height()),
		sideColor);
	Color backColor(.125, 1.);
	FillShader::Fill(
		Point(Screen::Left() + SIDEBAR_WIDTH + .5 * TEXT_WIDTH, 0.),
		Point(TEXT_WIDTH, Screen::Height()),
		backColor);
	Color lineColor(.2, 1.);
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
	Color dim = *GameData::Colors().Get("dim");
	Color medium = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
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
		if(dates[i].Month() == selected.Month())
		{
			FillShader::Fill(pos + highlightOffset - Point(1., 0.), highlightSize + Point(0., 2.), lineColor);
			FillShader::Fill(pos + highlightOffset, highlightSize, backColor);
		}
		font.Draw(contents[i], pos + textOffset, dates[i].Month() ? medium : bright);
		pos.Y() += LINE_HEIGHT;
	}
	
	// This should never happen, but bail out if there is no text to draw.
	if(begin == end)
		return;
	
	// Parameters for drawing the main text:
	WrappedText wrap;
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(TEXT_WIDTH - 2. * PAD);
	wrap.SetFont(font);
	
	// Draw the main text.
	pos = Screen::TopLeft() + Point(SIDEBAR_WIDTH + PAD, PAD + .5 * (LINE_HEIGHT - font.Height()) - scroll);
	for(auto it = end; it-- != begin; )
	{
		string date = it->first.ToString();
		double dateWidth = font.Width(date);
		font.Draw(date, pos + Point(TEXT_WIDTH - dateWidth - 2. * PAD, textOffset.Y()), dim);
		pos.Y() += LINE_HEIGHT;
		
		wrap.Wrap(it->second);
		wrap.Draw(pos, medium);
		pos.Y() += wrap.Height() + GAP;
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
		for( ; i < dates.size(); ++i)
			if(dates[i].Month() == selected.Month())
				break;
		
		if(key == SDLK_DOWN)
		{
			++i;
			if(i >= dates.size())
				return true;
		}
		else if(i)
		{
			--i;
			if(!i)
				return true;
			if(!dates[i].Month())
				--i;
		}
		selected = dates[i];
		Update();
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
			selected = dates[index];
			scroll = 0.;
			Update();
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



void LogbookPanel::Update()
{
	contents.clear();
	dates.clear();
	// The logbook should never be opened if it has no entries, but just in case:
	if(player.Logbook().empty())
	{
		contents.emplace_back("[No entries]");
		dates.emplace_back(selected);
		begin = end = player.Logbook().end();
		return;
	}
	
	// Check what years and months have entries for them.
	set<int> years;
	set<int> months;
	for(const auto &it : player.Logbook())
	{
		years.insert(it.first.Year());
		if(it.first.Year() == selected.Year() && it.first.Month() >= 1 && it.first.Month() <= 12)
			months.insert(it.first.Month());
	}
	
	// Generate the table of contents.
	for(auto yit = years.end(); yit-- != years.begin(); )
	{
		contents.emplace_back(to_string(*yit));
		dates.emplace_back(0, 0, *yit);
		if(*yit == selected.Year())
			for(auto mit = months.end(); mit-- != months.begin(); )
			{
				static const string MONTH[] = {
					"January", "February", "March", "April", "May", "June",
					"July", "August", "September", "October", "November", "Dec"};
				contents.emplace_back("  " + MONTH[*mit - 1]);
				dates.emplace_back(0, *mit, *yit);
			}
	}
	
	// Make sure a month is selected, within the current year.
	if(!selected.Month())
		selected = Date(0, *--months.end(), selected.Year());
	// Get the range of entries that include the selected month.
	begin = player.Logbook().lower_bound(Date(0, selected.Month(), selected.Year()));
	end = player.Logbook().lower_bound(Date(32, selected.Month(), selected.Year()));
}
