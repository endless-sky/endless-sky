/* PreferencesPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PreferencesPanel.h"

#include "Audio.h"
#include "Color.h"
#include "Dialog.h"
#include "Files.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Preferences.h"
#include "Screen.h"
#include "StarField.h"
#include "Table.h"
#include "UI.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

using namespace std;

namespace {
	// Settings that require special handling.
	static const string ZOOM_FACTOR = "Zoom factor";
	static const string EXPEND_AMMO = "Escorts expend ammo";
	static const string FRUGAL_ESCORTS = "Escorts use ammo frugally";
	static const string REACTIVATE_HELP = "Reactivate first-time help";
	static const string SCROLL_SPEED = "Scroll speed";
}



PreferencesPanel::PreferencesPanel()
	: editing(-1), selected(0), hover(-1)
{
	SetIsFullScreen(true);
}



// Draw this panel.
void PreferencesPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	
	Information info;
	info.SetBar("volume", Audio::Volume());
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	string pageName = (page == 'c' ? "controls" : page == 's' ? "settings" : "plugins");
	GameData::Interfaces().Get(pageName)->Draw(info, this);
	GameData::Interfaces().Get("preferences")->Draw(info, this);
	
	zones.clear();
	prefZones.clear();
	if(page == 'c')
		DrawControls();
	else if(page == 's')
		DrawSettings();
	else if(page == 'p')
		DrawPlugins();
}



bool PreferencesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(static_cast<unsigned>(editing) < zones.size())
	{
		Command::SetKey(zones[editing].Value(), key);
		EndEditing();
		return true;
	}
	
	if(key == SDLK_DOWN && static_cast<unsigned>(selected + 1) < zones.size())
		++selected;
	else if(key == SDLK_UP && selected > 0)
		--selected;
	else if(key == SDLK_RETURN)
		editing = selected;
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		Exit();
	else if(key == 'c' || key == 's' || key == 'p')
		page = key;
	else
		return false;
	
	return true;
}



bool PreferencesPanel::Click(int x, int y)
{
	EndEditing();
	
	if(x >= 265 && x < 295 && y >= -220 && y < 70)
	{
		Audio::SetVolume((20 - y) / 200.);
		Audio::Play(Audio::Get("warder"));
		return true;
	}
	
	Point point(x, y);
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
			editing = selected = index;
	
	for(const auto &zone : prefZones)
		if(zone.Contains(point))
		{
			if(zone.Value() == ZOOM_FACTOR)
			{
				int currentZoom = Screen::Zoom();
				int newZoom = currentZoom == 100 ? 150 : currentZoom == 150 ? 200 : 100;
				Screen::SetZoom(newZoom);
				// Make sure there is enough vertical space for the full UI.
				if(Screen::Height() < 700)
				{
					// Notify the user why setting the zoom any higher isn't permitted.
					GetUI()->Push(new Dialog("A zoom level of " + std::to_string(newZoom)
					+ "% would obscure important parts of the screen. Resetting to 100%."));
					Screen::SetZoom(100);
				}
				else {
					// Convert to raw window coordinates, at the new zoom level.
					point *= Screen::Zoom() / 100.;
					point += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
					SDL_WarpMouseInWindow(nullptr, point.X(), point.Y());
				}
			}
			if(zone.Value() == EXPEND_AMMO)
				Preferences::ToggleAmmoUsage();
			else if(zone.Value() == REACTIVATE_HELP)
			{
				for(const auto &it : GameData::HelpTemplates())
					Preferences::Set("help: " + it.first, false);
			}
			else if(zone.Value() == SCROLL_SPEED)
			{
				// Toogle between three different speeds.
				int speed = Preferences::ScrollSpeed() + 20;
				if(speed > 60)
					speed = 20;
				Preferences::SetScrollSpeed(speed);
			}
			else
				Preferences::Set(zone.Value(), !Preferences::Has(zone.Value()));
			break;
		}
	
	return true;
}



bool PreferencesPanel::Hover(int x, int y)
{
	Point point(x, y);
	
	hover = -1;
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
			hover = index;
	
	hoverPreference.clear();
	for(const auto &zone : prefZones)
		if(zone.Contains(point))
			hoverPreference = zone.Value();
	
	return true;
}



void PreferencesPanel::EndEditing()
{
	editing = -1;
}



void PreferencesPanel::DrawControls()
{
	Color back = *GameData::Colors().Get("faint");
	Color dim = *GameData::Colors().Get("dim");
	Color medium = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
	// Check for conflicts.
	Color red(.3, 0., 0., .3);
	
	Table table;
	table.AddColumn(-115, Table::LEFT);
	table.AddColumn(115, Table::RIGHT);
	table.SetUnderline(-120, 120);
	
	int firstY = -248;
	table.DrawAt(Point(-130, firstY));
	
	static const string CATEGORIES[] = {
		"Navigation",
		"Weapons",
		"Targeting",
		"Menus",
		"Fleet"
	};
	const string *category = CATEGORIES;
	static const Command COMMANDS[] = {
		Command::NONE,
		Command::FORWARD,
		Command::LEFT,
		Command::RIGHT,
		Command::BACK,
		Command::AFTERBURNER,
		Command::LAND,
		Command::JUMP,
		Command::NONE,
		Command::PRIMARY,
		Command::SELECT,
		Command::SECONDARY,
		Command::CLOAK,
		Command::NONE,
		Command::NEAREST,
		Command::TARGET,
		Command::HAIL,
		Command::BOARD,
		Command::SCAN,
		Command::NONE,
		Command::MENU,
		Command::MAP,
		Command::INFO,
		Command::FULLSCREEN,
		Command::NONE,
		Command::DEPLOY,
		Command::FIGHT,
		Command::GATHER,
		Command::HOLD,
		Command::AMMO
	};
	static const Command *BREAK = &COMMANDS[19];
	for(const Command &command : COMMANDS)
	{
		// The "BREAK" line is where to go to the next column.
		if(&command == BREAK)
			table.DrawAt(Point(130, firstY));
		
		if(!command)
		{
			table.DrawGap(10);
			table.DrawUnderline(medium);
			if(category != end(CATEGORIES))
				table.Draw(*category++, bright);
			else
				table.Advance();
			table.Draw("Key", bright);
			table.DrawGap(5);
		}
		else
		{
			int index = zones.size();
			// Mark conflicts.
			bool isConflicted = command.HasConflict();
			bool isEditing = (index == editing);
			if(isConflicted || isEditing)
			{
				table.SetHighlight(66, 120);
				table.DrawHighlight(isEditing ? dim: red);
			}
			
			// Mark the selected row.
			bool isHovering = (index == hover && !isEditing);
			if(!isHovering && index == selected)
			{
				table.SetHighlight(-120, 64);
				table.DrawHighlight(back);
			}
			
			// Highlight whichever row the mouse hovers over.
			table.SetHighlight(-120, 120);
			if(isHovering)
				table.DrawHighlight(back);
			
			zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), command);
			
			table.Draw(command.Description(), medium);
			table.Draw(command.KeyName(), isEditing ? bright : medium);
		}
	}
	
	Table shiftTable;
	shiftTable.AddColumn(125, Table::RIGHT);
	shiftTable.SetUnderline(0, 130);
	shiftTable.DrawAt(Point(-400, 52));
	
	shiftTable.DrawUnderline(medium);
	shiftTable.Draw("With <shift> key", bright);
	shiftTable.DrawGap(5);
	shiftTable.Draw("Select nearest ship", medium);
	shiftTable.Draw("Select next escort", medium);
	shiftTable.Draw("Talk to planet", medium);
	shiftTable.Draw("Board disabled escort", medium);
}



void PreferencesPanel::DrawSettings()
{
	Color back = *GameData::Colors().Get("faint");
	Color dim = *GameData::Colors().Get("dim");
	Color medium = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
	Table table;
	table.AddColumn(-115, Table::LEFT);
	table.AddColumn(115, Table::RIGHT);
	table.SetUnderline(-120, 120);
	
	int firstY = -248;
	table.DrawAt(Point(-130, firstY));
	
	static const string SETTINGS[] = {
		"Display",
		ZOOM_FACTOR,
		"Show status overlays",
		"Show planet labels",
		"Show mini-map",
		"",
		"AI",
		"Automatic aiming",
		"Automatic firing",
		EXPEND_AMMO,
		"",
		"Performance",
		"Show CPU / GPU load",
		"Render motion blur",
		"Reduce large graphics",
		"Draw background haze",
		"Show hyperspace flash",
		"\n",
		"Other",
		REACTIVATE_HELP,
		SCROLL_SPEED,
		"Warning siren",
		"Hide unexplored map regions"
	};
	bool isCategory = true;
	for(const string &setting : SETTINGS)
	{
		// Check if this is a category break or column break.
		if(setting.empty() || setting == "\n")
		{
			isCategory = true;
			if(!setting.empty())
				table.DrawAt(Point(130, firstY));
			continue;
		}
		
		if(isCategory)
		{
			isCategory = false;
			table.DrawGap(10);
			table.DrawUnderline(medium);
			table.Draw(setting, bright);
			table.Advance();
			table.DrawGap(5);
			continue;
		}
		
		// Record where this setting is displayed, so the user can click on it.
		prefZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), setting);
		
		// Get the "on / off" text for this setting.
		bool isOn = Preferences::Has(setting);
		string text;
		if(setting == ZOOM_FACTOR)
		{
			isOn = true;
			text = to_string(Screen::Zoom());
		}
		else if(setting == EXPEND_AMMO)
			text = Preferences::AmmoUsage();
		else if(setting == REACTIVATE_HELP)
		{
			// Check how many help messages have been displayed.
			const map<string, string> &help = GameData::HelpTemplates();
			int shown = 0;
			for(const auto &it : help)
				shown += Preferences::Has("help: " + it.first);
			
			if(shown)
				text = to_string(shown) + " / " + to_string(help.size());
			else
			{
				isOn = true;
				text = "done";
			}
		}
		else if(setting == SCROLL_SPEED)
			text = to_string(Preferences::ScrollSpeed());
		else
			text = isOn ? "on" : "off";
		
		if(setting == hoverPreference)
			table.DrawHighlight(back);
		table.Draw(setting, isOn ? medium : dim);
		table.Draw(text, isOn ? bright : medium);
	}
}



void PreferencesPanel::DrawPlugins()
{
	// TODO.
}



void PreferencesPanel::Exit()
{
	Command::SaveSettings(Files::Config() + "keys.txt");
	
	GetUI()->Pop(this);
}
