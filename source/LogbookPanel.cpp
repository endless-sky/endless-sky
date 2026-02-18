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
#include "shader/LineShader.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "shader/RingShader.h"
#include "Screen.h"
#include "image/SpriteLoadManager.h"
#include "image/SpriteSet.h"
#include "System.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <ranges>

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
	// Load any and deferred scenes that appear in the logbook.
	// This is done here instead of in the constructor because the constructor
	// does not have access to the UI stack.
	if(!hasLoadedScenes)
	{
		hasLoadedScenes = true;
		for(const auto &entry : player.Logbook())
			for(const Sprite *scene : entry.second.GetScenes())
				SpriteLoadManager::LoadDeferred(GetUI().AsyncQueue(), scene);
	}
}



void LogbookPanel::Draw()
{
	MapPanel::Draw();
	DrawOrbits();
	DrawKey();
	DrawSelectedEntry();
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
		vector<Selection> selections = AvailableSelections(false);
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
			selectedEntry = nullptr;

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

	if(x - Screen::Left() > WIDTH)
		return MapPanel::Click(x, y, button, clicks);

	Point clickPoint(x, y);
	for(const ClickZone<Selection> &zone : selectionZones)
		if(zone.Contains(clickPoint))
		{
			selection = zone.Value();
			scroll = 0.;
			selectedEntry = nullptr;
			UI::PlaySound(UI::UISound::NORMAL);
			return true;
		}
	for(const ClickZone<const BookEntry *> &zone : logZones)
		if(zone.Contains(clickPoint))
		{
			SelectEntry(*zone.Value());
			UI::PlaySound(UI::UISound::NORMAL);
			return true;
		}

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



LogbookPanel::Entry::Entry(EntryType type, const string &heading, const BookEntry &body)
	: type(type), heading(heading), body(body)
{
}



LogbookPanel::Entry::Entry(EntryType type, const PlayerInfo::StorylineProgress &progress)
	: type(type), heading(progress.Heading()), subheading(progress.Subheading()),
		body(progress.GetBookEntry())
{
}



void LogbookPanel::CreateSections()
{
	sections.clear();
	// Special logs aren't expandable, and so they have the same category and subcategory name.
	for(const auto &[category, entries] : player.SpecialLogs())
	{
		Section &section = sections[category];
		Page &page = section.insert({category, Page(PageType::SPECIAL)}).first->second;
		for(const auto &[heading, body] : entries)
			page.entries.emplace_back(EntryType::NORMAL, heading, body);
	}
	// Each storyline has its own section and a page for each book.
	for(const auto &storyline : player.GetStorylineProgress() | views::values)
	{
		// All storyline progress entries require that the defined storyline
		// object is present. This way, storyline components that no longer
		// exist can be removed from the logbook, allowing us to create
		// temporary entries for things like "to be continued" entries
		// while a storyline is still being written.
		if(!storyline.BackingEntry())
			continue;
		const string &sectionName = storyline.SectionName();
		Section &section = sections[sectionName];
		// If a storyline has no books, at least display the storyline's log.
		if(storyline.Children().empty())
		{
			Page &page = section.insert({sectionName, Page(PageType::STORYLINE)}).first->second;
			page.entries.emplace_back(EntryType::STORYLINE, storyline);
			continue;
		}
		for(const auto &book : storyline.Children() | views::values)
		{
			if(!book.BackingEntry())
				continue;
			// Each book is a separate page, headed by the storyline and book logs,
			// then breaking down into each arc and chapter.
			Page &page = section.insert({book.SectionName(), Page(PageType::STORYLINE)}).first->second;
			page.entries.emplace_back(EntryType::STORYLINE, storyline);
			page.entries.emplace_back(EntryType::BOOK, book);
			for(const auto &arc : book.Children() | views::values)
			{
				if(!arc.BackingEntry())
					continue;
				page.entries.emplace_back(EntryType::ARC, arc);
				for(const auto &chapter : arc.Children() | views::values)
				{
					if(!chapter.BackingEntry())
						continue;
					page.entries.emplace_back(EntryType::CHAPTER, chapter);
				}
			}
		}
	}
	// For the rest of the logbook, the category is the year, the subcategory is the month.
	for(const auto &[date, entry] : player.Logbook())
	{
		Section &section = sections[to_string(date.Year())];
		Page &page = section.insert({MONTH[date.Month() - 1], Page(PageType::DATE)}).first->second;
		page.entries.emplace_back(EntryType::NORMAL, date.ToString(), entry);
	}
}



void LogbookPanel::SelectEntry(const BookEntry &entry)
{
	vector<const System *> options;
	set<const System *> uniqueOptions;

	auto AddOption = [&](const System *system) -> void {
		if(!uniqueOptions.contains(system) && system->IsValid() && player.CanView(*system))
		{
			options.push_back(system);
			uniqueOptions.insert(system);
		}
	};

	if(entry.SourceSystem())
		AddOption(entry.SourceSystem());
	for(const System *system : entry.MarkSystems())
		AddOption(system);
	for(const System *system : entry.CircleSystems())
		AddOption(system);

	if(options.empty())
		centeredSystem = nullptr;
	else if(selectedEntry != &entry)
		centeredSystem = options.front();
	else if(options.size() > 1)
	{
		auto it = std::next(ranges::find(options, centeredSystem));
		centeredSystem = it == options.end() ? options.front() : *it;
	}
	if(centeredSystem)
		CenterOnSystem(centeredSystem);
	selectedEntry = &entry;
}



void LogbookPanel::DrawSelectedEntry() const
{
	if(!selectedEntry)
		return;

	const Set<Color> &colors = GameData::Colors();
	const Color &sourceColor = *colors.Get("active mission");
	const Color &markColor = *colors.Get("waypoint");

	double zoom = Zoom();
	const Color black(0.f, 1.f);
	Point angle = Angle(30.).Unit();

	auto DrawPointer = [&](const System *system, const Color &color) -> void {
		Point position = zoom * (system->Position() + center);
		PointerShader::Add(position, angle, 14.f, 19.f, -4.f, black);
		PointerShader::Add(position, angle, 8.f, 15.f, -6.f, color);
	};
	auto DrawRing = [&](const System *system, const Color &color) -> void {
		RingShader::Add(zoom * (system->Position() + center), 22.f, 20.5f, color);
	};

	PointerShader::Bind();
	{
		const System *source = selectedEntry->SourceSystem();
		if(source && source->IsValid() && player.CanView(*source))
			DrawPointer(source, sourceColor);
		for(const System *system : selectedEntry->MarkSystems())
			if(system->IsValid() && player.CanView(*system))
				DrawPointer(system, markColor);
	}
	PointerShader::Unbind();
	RingShader::Bind();
	{
		for(const System *system : selectedEntry->CircleSystems())
			if(system->IsValid() && player.CanView(*system))
				DrawRing(system, markColor);
	}
	RingShader::Unbind();
}



void LogbookPanel::DrawLogbook()
{
	selectionZones.clear();
	logZones.clear();

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
	Color entryHover = bright.Transparent(.125);
	Color entrySelected = medium.Transparent(.125);

	// Draw the sidebar.
	// The currently selected sidebar item should be highlighted. This is how
	// big the highlight rectangle is.
	Point highlightSize(SIDEBAR_WIDTH - 4., LINE_HEIGHT);
	Point highlightOffset = Point(4. - PAD, 0.) + .5 * highlightSize;
	Point textOffset(0., .5 * (LINE_HEIGHT - font.Height()));
	// Start at this point on the screen:
	Point pos = Screen::TopLeft() + Point(PAD, PAD - categoryScroll);
	Point zoneStart;
	Point zoneSize = Point(SIDEBAR_WIDTH, LINE_HEIGHT);
	for(const auto &[name, section] : sections)
	{
		zoneStart = pos;
		bool isExpandable = section.size() != 1 || name != section.front().first;
		if(selection.first == name && !isExpandable)
		{
			FillShader::Fill(pos + highlightOffset - Point(1., 0.), highlightSize + Point(0., 2.), lineColor);
			FillShader::Fill(pos + highlightOffset, highlightSize, backColor);
		}
		font.Draw(name, pos + textOffset, bright);
		pos.Y() += LINE_HEIGHT;
		selectionZones.emplace_back(Rectangle::FromCorner(zoneStart, zoneSize), make_pair(name, section.back().first));
		if(selection.first != name || !isExpandable)
			continue;

		for(const auto &pageName : section | views::keys)
		{
			zoneStart = pos;
			if(selection.second == pageName)
			{
				FillShader::Fill(pos + highlightOffset - Point(1., 0.), highlightSize + Point(0., 2.), lineColor);
				FillShader::Fill(pos + highlightOffset, highlightSize, backColor);
			}
			font.Draw(pageName, pos + textOffset, medium);
			pos.Y() += LINE_HEIGHT;
			selectionZones.emplace_back(Rectangle::FromCorner(zoneStart, zoneSize), make_pair(name, pageName));
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
		zoneStart = pos - Point(.5 * PAD, 0);
		// Chapters are drawn slightly indented.
		if(entry.type == EntryType::CHAPTER)
		{
			wrap.SetWrapWidth(TEXT_WIDTH - 4. * PAD);
			pos += Point(2 * PAD, 0);
		}

		// Storyline pages have a heading and subheading on entries.
		// Dates and special pages only have a heading.
		// The date page headings are shifted to the right and dimmed.
		if(page.type == PageType::STORYLINE)
		{
			font.Draw(entry.heading, pos + textOffset, bright);
			pos.Y() += LINE_HEIGHT;
			if(entry.type == EntryType::CHAPTER)
				font.Draw({entry.subheading, layout}, pos + Point(-2 * PAD, textOffset.Y()), dim);
			else
				font.Draw({entry.subheading, layout}, pos + Point(0., textOffset.Y()), dim);
		}
		else if(page.type == PageType::DATE)
			font.Draw({entry.heading, layout}, pos + Point(0., textOffset.Y()), dim);
		else
			font.Draw(entry.heading, pos + textOffset, bright);
		pos.Y() += LINE_HEIGHT;
		pos.Y() += entry.body.Draw(pos, wrap, medium);

		// Storylines and books have a line drawn under them.
		Point right = Point(pos.X() + TEXT_WIDTH - PAD, pos.Y());
		if(entry.type == EntryType::STORYLINE)
			LineShader::Draw(pos, right, 1, bright);
		else if(entry.type == EntryType::BOOK)
			LineShader::Draw(pos, right, 1, medium);

		zoneSize = Point(TEXT_WIDTH, pos.Y() - zoneStart.Y());
		if(entry.body.HasSystems())
		{
			logZones.emplace_back(Rectangle::FromCorner(zoneStart, zoneSize), &entry.body);
			ClickZone<const BookEntry *> zone = logZones.back();
			if(zone.Contains(hoverPoint))
				FillShader::Fill(zone, entryHover);
			else if(selectedEntry == &entry.body)
				FillShader::Fill(zone, entrySelected);
		}

		pos.Y() += GAP;

		// Un-indent from the chapter.
		if(entry.type == EntryType::CHAPTER)
		{
			wrap.SetWrapWidth(TEXT_WIDTH - 2. * PAD);
			pos -= Point(2 * PAD, 0);
		}
	}

	maxScroll = max(0., scroll + pos.Y() - Screen::Bottom());
}



vector<pair<string, string>> LogbookPanel::AvailableSelections(bool visibleOnly) const
{
	vector<Selection> selections;
	for(const auto &[name, section] : sections)
	{
		if(section.size() == 1 && section.front().first == name)
			selections.emplace_back(name, name);
		else if(!visibleOnly || selection.first == name)
		{
			selections.emplace_back(make_pair("", ""));
			for(const auto &pageName : section | views::keys)
				selections.emplace_back(name, pageName);
		}
		else
			selections.emplace_back(name, section.back().first);
	}
	return selections;
}
