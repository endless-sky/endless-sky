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
#include "MainPanel.h"
#include "ShipyardPanel.h"
#include "UI.h"

#include <boost/filesystem.hpp>

#include <algorithm>

namespace fs = boost::filesystem3;

using namespace std;



LoadPanel::LoadPanel(const GameData &data, PlayerInfo &player, UI &gamePanels)
	: data(data), player(player), gamePanels(gamePanels), loadedInfo(player)
{
	UpdateLists();
}



void LoadPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	data.Background().Draw(Point(), Point());
	
	Information info;
	if(loadedInfo.IsLoaded())
	{
		info.SetString("pilot", loadedInfo.FirstName() + " " + loadedInfo.LastName());
		if(player.GetShip())
		{
			const Ship &ship = *loadedInfo.GetShip();
			info.SetSprite("ship sprite", ship.GetSprite().GetSprite());
			info.SetString("ship", ship.Name());
		}
		if(loadedInfo.GetSystem())
			info.SetString("system", loadedInfo.GetSystem()->Name());
		if(loadedInfo.GetPlanet())
			info.SetString("planet", loadedInfo.GetPlanet()->Name());
		info.SetString("credits", to_string(loadedInfo.Accounts().Credits()));
		info.SetString("date", loadedInfo.GetDate().ToString());
	}
	else
		info.SetString("pilot", "No Pilot Loaded");
	
	if(!selectedPilot.empty())
		info.SetCondition("pilot selected");
	if(selectedFile.find('~') != string::npos)
		info.SetCondition("snapshot selected");
	if(loadedInfo.IsLoaded())
		info.SetCondition("pilot loaded");
	
	const Interface *menu = data.Interfaces().Get("load menu");
	menu->Draw(info);
	
	Color dim(.1, 0.);
	Color grey(.5, 0.);
	const Font &font = FontSet::Get(14);
	
	Point point(-460., -157.);
	for(const auto &it : files)
	{
		if(it.first == selectedPilot)
			FillShader::Fill(point + Point(100., 7.), Point(210., 20.), dim);
		font.Draw(it.first, point, grey);
		point += Point(0., 20.);
	}
	
	if(!selectedPilot.empty())
	{
		point = Point(-100., -157.);
		for(const string &file : files.find(selectedPilot)->second)
		{
			if(file == selectedFile)
				FillShader::Fill(point + Point(100., 7.), Point(210., 20.), dim);
			font.Draw(file.substr(0, file.size() - 4), point, grey);
			point += Point(0., 20.);
		}
	}
}



// New player "conversation" callback.
void LoadPanel::OnCallback(int)
{
	GetUI()->Pop(this);
	GetUI()->Pop(GetUI()->Root().get());
	gamePanels.Reset();
	Panel *panel = new MainPanel(data, player);
	gamePanels.Push(panel);
	// Tell the main panel to re-draw itself (and pop up the planet panel).
	panel->Step(true);
	gamePanels.Push(new ShipyardPanel(data, player));
}



bool LoadPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == 'n')
	{
		player.New(data);
		
		ConversationPanel *panel = new ConversationPanel(player, *data.Conversations().Get("intro"));
		GetUI()->Push(panel);
		panel->SetCallback(*this);
	}
	else if(key == 'd')
	{
		auto it = files.find(selectedPilot);
		if(it == files.end())
			return false;
		
		for(const string &file : it->second)
			fs::remove(root + file);
		UpdateLists();
	}
	else if(key == 'a')
	{
		string wasSelected = selectedPilot;
		auto it = files.find(selectedPilot);
		if(it == files.end() || it->second.empty() || it->second.front().size() < 4)
			return false;
		
		// Extract the date from this pilot's most recent save.
		string date = "~0000-00-00.txt";
		string from = root + it->second.front();
		DataFile file(from);
		for(const DataFile::Node &node : file)
			if(node.Token(0) == "date")
			{
				int year = node.Value(3);
				int month = node.Value(2);
				int day = node.Value(1);
				date[1] += (year / 1000) % 10;
				date[2] += (year / 100) % 10;
				date[3] += (year / 10) % 10;
				date[4] += year % 10;
				date[6] += (month / 10) % 10;
				date[7] += month % 10;
				date[9] += (day / 10) % 10;
				date[10] += day % 10;
			}
		
		// Copy the autosave to a new, named file.
		string to = from.substr(0, from.size() - 4) + date;
		fs::copy(from, to);
		UpdateLists();
		
		selectedPilot = wasSelected;
		selectedFile = to.substr(root.length());
	}
	else if(key == 'r')
	{
		fs::remove(root + selectedFile);
		UpdateLists();
		
		auto it = files.find(selectedPilot);
		if(it == files.end() || it->second.empty())
			selectedFile.clear();
		else
			selectedFile = it->second.front();
	}
	else if(key == 'e')
	{
		player = loadedInfo;
		GetUI()->Pop(this);
		GetUI()->Pop(GetUI()->Root().get());
		gamePanels.Reset();
		gamePanels.Push(new MainPanel(data, player));
	}
	else if(key == 'b' || key == data.Keys().Get(Key::MENU))
		GetUI()->Pop(this);
	
	return true;
}



bool LoadPanel::Click(int x, int y)
{
	char key = data.Interfaces().Get("load menu")->OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDLKey>(key), KMOD_NONE);
	
	// The first row of each panel is y = -160 to -140.
	if(y < -160)
		return false;
	int selected = (y - -160) / 20;
	
	if(x >= -460 && x < -260)
	{
		int i = 0;
		for(const auto &it : files)
			if(i++ == selected && selectedPilot != it.first)
			{
				selectedPilot = it.first;
				selectedFile = it.second.front();
			}
	}
	else if(x >= -100 && x < 100)
	{
		int i = 0;
		for(const string &file : files.find(selectedPilot)->second)
			if(i++ == selected && selectedFile != file)
				selectedFile = file;
	}
	else
		return false;
	
	loadedInfo.Load(root + selectedFile, data);
	
	return true;
}



void LoadPanel::UpdateLists()
{
	files.clear();
	selectedPilot.clear();
	selectedFile.clear();
	
	root = getenv("HOME") + string("/.config/endless-sky/saves/");
	fs::directory_iterator it(root);
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
	
	for(auto &it : files)
		sort(it.second.begin(), it.second.end());
}
