/* LoadPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "LoadPanel.h"

#include "Color.h"
#include "ConversationPanel.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "ShipyardPanel.h"
#include "UI.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

using namespace std;



LoadPanel::LoadPanel(const GameData &data, PlayerInfo &player, UI &gamePanels)
	: data(data), player(player), gamePanels(gamePanels)
{
	string path = getenv("HOME") + string("/.config/endless-sky/saves/");
	fs::directory_iterator it(path);
	fs::directory_iterator end;
	for( ; it != end; ++it)
	{
		string fileName = it->path().filename().string();
		// The file name is either "Pilot Name.txt" or "Pilot Name~Date.txt".
		size_t pos = fileName.find('~');
		if(pos == string::npos)
			pos = fileName.size() - 4;
		
		string pilotName = fileName.substr(0, pos);
		files[pilotName].push_back(fileName);
	}
	
	if(!files.empty())
	{
		selectedPilot = files.begin()->first;
		selectedFile = files.begin()->second.front();
	}
}



void LoadPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	data.Background().Draw(Point(), Point());
	
	Information info;
	if(player.IsLoaded())
	{
		info.SetString("pilot", player.FirstName() + " " + player.LastName());
		if(player.GetShip())
		{
			const Ship &ship = *player.GetShip();
			info.SetSprite("ship sprite", ship.GetSprite().GetSprite());
			info.SetString("ship", ship.Name());
		}
		if(player.GetSystem())
			info.SetString("system", player.GetSystem()->Name());
		if(player.GetPlanet())
			info.SetString("planet", player.GetPlanet()->Name());
		info.SetString("credits", to_string(player.Accounts().Credits()));
		info.SetString("date", player.GetDate().ToString());
	}
	else
		info.SetString("pilot", "No Pilot Loaded");
	
	const Interface *menu = data.Interfaces().Get("load menu");
	menu->Draw(info);
	
	Color dim(.1, 0.);
	Color grey(.5, 0.);
	const Font &font = FontSet::Get(14);
	
	Point point(-460., -157.);
	for(const auto &it : files)
	{
		if(it.first == selectedPilot)
			FillShader::Fill(point + Point(100., 7.), Point(200., 20.), dim.Get());
		font.Draw(it.first, point, grey.Get());
		point += Point(0., 20.);
	}
	
	if(!selectedPilot.empty())
	{
		point = Point(-100., -157.);
		for(const string &file : files.find(selectedPilot)->second)
		{
			if(file == selectedFile)
				FillShader::Fill(point + Point(100., 7.), Point(200., 20.), dim.Get());
			font.Draw(file, point, grey.Get());
			point += Point(0., 20.);
		}
	}
}



// New player "conversation" callback.
void LoadPanel::OnCallback(int)
{
	GetUI()->Pop(this);
	GetUI()->Pop(GetUI()->Root().get());
	shared_ptr<Panel> saved = gamePanels.Root();
	gamePanels.Reset();
	gamePanels.Push(saved);
	// Tell the main panel to re-draw itself (and pop up the planet panel).
	saved->Step(true);
	gamePanels.Push(new ShipyardPanel(data, player));
}



bool LoadPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == 'b' || key == data.Keys().Get(Key::MENU))
		GetUI()->Pop(this);
	else if(key == 'n')
	{
		player.New(data);
		
		ConversationPanel *panel = new ConversationPanel(player, *data.Conversations().Get("intro"));
		GetUI()->Push(panel);
		panel->SetCallback(*this);
	}
	
	return true;
}



bool LoadPanel::Click(int x, int y)
{
	char key = data.Interfaces().Get("load menu")->OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDLKey>(key), KMOD_NONE);
	
	// TODO: handle clicks on lists of pilots / saves.
	// The first row of each panel is y = -160 to -140.
	
	return true;
}
