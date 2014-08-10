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
	
	Color dim = *GameData::Colors().Get("dim");
	Color medium = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
	Table table;
	table.AddColumn(-100, Table::RIGHT);
	table.AddColumn(-20, Table::RIGHT);
	table.AddColumn(0, Table::LEFT);
	table.SetUnderline(-200, 200);
	
	firstY = -10 * (Key::END - Key::MENU);
	table.DrawAt(Point(0., firstY - 25));
	
	table.DrawUnderline(medium);
	table.Draw("Default", medium);
	table.Draw("Key", medium);
	table.Draw("Action", medium);
	table.DrawGap(5);
	
	// Check for conflicts.
	Color red(.3, 0., 0., .3);
	map<int, int> count;
	for(Key::Command c = Key::MENU; c != Key::END; c = static_cast<Key::Command>(c + 1))
		++count[GameData::Keys().Get(c)];
	
	for(Key::Command c = Key::MENU; c != Key::END; c = static_cast<Key::Command>(c + 1))
	{
		string current = SDL_GetKeyName(static_cast<SDL_Keycode>(GameData::Keys().Get(c)));
		string def = SDL_GetKeyName(static_cast<SDL_Keycode>(GameData::DefaultKeys().Get(c)));

		int index = c - Key::MENU;
		// Mark conflicts.
		bool isConflicted = (count[GameData::Keys().Get(c)] > 1);
		if(isConflicted || index == editing)
		{
			table.SetHighlight(-70, -10);
			table.DrawHighlight(isConflicted ? red : dim);
			table.SetHighlight(-200, 200);
		}
		// Mark the selected row.
		if(index == selected)
			table.DrawHighlight(dim);
		
		table.Draw(def, current == def ? dim : medium);
		table.Draw(current, bright);
		table.Draw(Key::Description(c), medium);
	}
}



bool PreferencesPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(editing != -1)
	{
		GameData::SetKey(static_cast<Key::Command>(editing), key);
		editing = -1;
		return true;
	}
	
	if(key == SDLK_DOWN)
		selected += (selected != Key::END - 1);
	else if(key == SDLK_UP)
		selected -= (selected != Key::MENU);
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
	char key = GameData::Interfaces().Get("preferences")->OnClick(Point(x, y));
	if(key != '\0')
	{
		editing = -1;
		return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	}
	
	if(x < -210)
		return false;
	if(x >= 220 && x < 250 && y >= -190 && y < 90)
	{
		Audio::SetVolume((50 - y) / 200.);
		return true;
	}
	if(x >= 210)
		return false;
	
	y -= firstY;
	if(y < 0)
		return false;
	
	y /= 20;
	if(y >= Key::END)
		return false;
	
	selected = y;
	if(x >= -70 && x < -10)
		editing = selected;
	else if(x >= -150 && x < -90)
	{
		Key::Command command = static_cast<Key::Command>(selected);
		GameData::SetKey(command, GameData::DefaultKeys().Get(command));
	}
	
	return true;
}



void PreferencesPanel::Exit()
{
	string keysPath = getenv("HOME") + string("/.config/endless-sky/keys.txt");
	GameData::Keys().Save(keysPath);
	
	GetUI()->Pop(this);
}
