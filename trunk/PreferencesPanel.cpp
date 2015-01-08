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
#include "Files.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Preferences.h"
#include "Table.h"
#include "UI.h"

#include "gl_header.h"

#include <cmath>
#include <map>

using namespace std;

namespace {
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
		Command::SCAN,
		Command::HAIL,
		Command::NONE,
		Command::MENU,
		Command::MAP,
		Command::INFO,
		Command::FULLSCREEN,
		Command::NONE,
		Command::DEPLOY,
		Command::FIGHT,
		Command::GATHER,
		Command::HOLD
	};
	static const string CATEGORIES[] = {
		"Navigation",
		"Weapons",
		"Targeting",
		"Menus",
		"Fleet"
	};
	static const string SETTINGS[] = {
		"Show CPU / GPU load",
		"",
		"Automatic firing",
		"Automatic aiming"
	};
	static const Command *BREAK = &COMMANDS[18];
}



PreferencesPanel::PreferencesPanel()
	: editing(-1), selected(0)
{
	SetIsFullScreen(true);
}



// Draw this panel.
void PreferencesPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	
	const Interface *menu = GameData::Interfaces().Get("preferences");
	Information info;
	info.SetBar("volume", Audio::Volume());
	menu->Draw(info);
	
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
	
	int firstY = -240;
	table.DrawAt(Point(-130, firstY));
	
	Point endPoint;
	const string *category = CATEGORIES;
	zones.clear();
	for(const Command &command : COMMANDS)
	{
		// The "BREAK" line is where to go to the next column.
		if(&command == BREAK)
		{
			endPoint = table.GetPoint() + Point(260, -20);
			table.DrawAt(Point(130, firstY));
		}
		
		if(command == Command::NONE)
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
			if(index == selected)
			{
				table.SetHighlight(-120, 64);
				table.DrawHighlight(back);
			}
			
			table.SetHighlight(-120, 120);
			zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), command);
			
			table.Draw(command.Description(), medium);
			table.Draw(command.KeyName(), isEditing ? bright : medium);
		}
	}
	
	table.DrawAt(endPoint);
	prefZones.clear();
	for(const string &setting : SETTINGS)
	{
		if(setting.empty())
		{
			table.DrawGap(-10);
			continue;
		}
		
		prefZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), setting);
		
		bool isOn = Preferences::Has(setting);
		table.Draw(setting, isOn ? medium : dim);
		table.Draw(isOn ? "on" : "off", isOn ? bright : medium);
		table.DrawGap(-40);
	}
}



bool PreferencesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(static_cast<unsigned>(editing) < zones.size())
	{
		Command::SetKey(zones[editing].Value(), key);
		editing = -1;
		return true;
	}
	
	if(key == SDLK_DOWN && static_cast<unsigned>(selected + 1) < zones.size())
		++selected;
	else if(key == SDLK_UP && selected > 0)
		--selected;
	else if(key == SDLK_RETURN)
		editing = selected;
	else if(key == 'b' || command == Command::MENU)
		Exit();
	else
		return false;
	
	return true;
}



bool PreferencesPanel::Click(int x, int y)
{
	editing = -1;
	
	Point point(x, y);
	char key = GameData::Interfaces().Get("preferences")->OnClick(point);
	if(key)
		return DoKey(key);
	
	if(x >= 265 && x < 295 && y >= -220 && y < 70)
	{
		Audio::SetVolume((20 - y) / 200.);
		return true;
	}
	
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
			editing = selected = index;
	
	for(const auto &zone : prefZones)
		if(zone.Contains(point))
			Preferences::Set(zone.Value(), !Preferences::Has(zone.Value()));
	
	return true;
}



void PreferencesPanel::Exit()
{
	string keysPath = getenv("HOME") + string("/.config/endless-sky/keys.txt");
	Command::SaveSettings(Files::Config() + "keys.txt");
	
	GetUI()->Pop(this);
}
