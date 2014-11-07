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
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Table.h"
#include "UI.h"

#ifdef __APPLE__
#include <OpenGL/GL3.h>
#else
#include <GL/glew.h>
#endif

#include <cmath>
#include <map>

using namespace std;

namespace {
	static const Key::Command COMMANDS[] = {
		Key::END,
		Key::FORWARD,
		Key::LEFT,
		Key::RIGHT,
		Key::BACK,
		Key::AFTERBURNER,
		Key::LAND,
		Key::JUMP,
		Key::END,
		Key::PRIMARY,
		Key::SELECT,
		Key::SECONDARY,
		Key::CLOAK,
		Key::END,
		Key::NEAREST,
		Key::TARGET,
		Key::SCAN,
		Key::HAIL,
		Key::END,
		Key::MENU,
		Key::MAP,
		Key::INFO,
		Key::FULLSCREEN,
		Key::END,
		Key::DEPLOY,
		Key::FIGHT,
		Key::GATHER,
		Key::HOLD
	};
	static const string CATEGORIES[] = {
		"Navigation",
		"Weapons",
		"Targeting",
		"Menus",
		"Fleet"
	};
	static const Key::Command *BREAK = &COMMANDS[18];
}



PreferencesPanel::PreferencesPanel()
	: editing(-1), selected(0), firstY(0)
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
	Color medium = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
	// Check for conflicts.
	Color red(.3, 0., 0., .3);
	map<int, int> count;
	for(Key::Command c = Key::MENU; c != Key::END; c = static_cast<Key::Command>(c + 1))
		++count[GameData::Keys().Get(c)];
	
	Table table;
	table.AddColumn(-115, Table::LEFT);
	table.AddColumn(115, Table::RIGHT);
	table.SetUnderline(-120, 120);
	
	firstY = -240;
	table.DrawAt(Point(-130, -240));
	
	const string *category = CATEGORIES;
	zones.clear();
	for(const Key::Command &command : COMMANDS)
	{
		// The "BREAK" line is where to go to the next column.
		if(&command == BREAK)
			table.DrawAt(Point(130, firstY));
		
		if(command == Key::END)
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
			SDL_Keycode key = static_cast<SDL_Keycode>(GameData::Keys().Get(command));
			string current = SDL_GetKeyName(key);
			
			int index = zones.size();
			// Mark conflicts.
			bool isConflicted = (count[key] > 1);
			if(isConflicted || index == editing)
			{
				table.SetHighlight(65, 120);
				table.DrawHighlight(isConflicted ? red : back);
				table.SetHighlight(-120, 120);
			}
			// Mark the selected row.
			if(index == selected)
				table.DrawHighlight(back);
			
			zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), command);
			
			table.Draw(Key::Description(command), medium);
			table.Draw(current, bright);
		}
	}
}



bool PreferencesPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(static_cast<unsigned>(editing) < zones.size())
	{
		GameData::SetKey(zones[editing].Value(), key);
		editing = -1;
		return true;
	}
	
	if(key == SDLK_DOWN && static_cast<unsigned>(selected + 1) < zones.size())
		++selected;
	else if(key == SDLK_UP && selected > 0)
		--selected;
	else if(key == SDLK_RETURN)
		editing = selected;
	else if(key == 'b' || key == GameData::Keys().Get(Key::MENU))
		Exit();
	else
		return false;
	
	return true;
}



bool PreferencesPanel::Click(int x, int y)
{
	Point point(x, y);
	char key = GameData::Interfaces().Get("preferences")->OnClick(point);
	if(key != '\0')
	{
		editing = -1;
		return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	}
	
	if(x >= 265 && x < 295 && y >= -220 && y < 70)
	{
		Audio::SetVolume((20 - y) / 200.);
		return true;
	}
	
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
			editing = selected = index;
	
	return true;
}



void PreferencesPanel::Exit()
{
	string keysPath = getenv("HOME") + string("/.config/endless-sky/keys.txt");
	GameData::Keys().Save(keysPath);
	
	GetUI()->Pop(this);
}
