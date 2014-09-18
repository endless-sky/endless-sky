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
#include "DataFile.h"
#include "Dialog.h"
#include "Files.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "MainPanel.h"
#include "Messages.h"
#include "ShipyardPanel.h"
#include "UI.h"

#include <algorithm>

using namespace std;



LoadPanel::LoadPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), selectedPilot(player.Identifier())
{
	UpdateLists();
}



void LoadPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	
	Information info;
	if(loadedInfo.IsLoaded())
	{
		info.SetString("pilot", loadedInfo.FirstName() + " " + loadedInfo.LastName());
		if(loadedInfo.GetShip())
		{
			const Ship &ship = *loadedInfo.GetShip();
			info.SetSprite("ship sprite", ship.GetSprite().GetSprite());
			info.SetString("ship", ship.Name());
		}
		if(loadedInfo.GetSystem())
			info.SetString("system", loadedInfo.GetSystem()->Name());
		if(loadedInfo.GetPlanet())
			info.SetString("planet", loadedInfo.GetPlanet()->Name());
		info.SetString("credits", Format::Number(loadedInfo.Accounts().Credits()));
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
	
	const Interface *menu = GameData::Interfaces().Get("load menu");
	menu->Draw(info);
	
	Color dim = *GameData::Colors().Get("faint");
	Color grey = *GameData::Colors().Get("medium");
	const Font &font = FontSet::Get(14);
	
	Point point(-460., -157.);
	for(const auto &it : files)
	{
		if(it.first == selectedPilot)
			FillShader::Fill(point + Point(100., 7.), Point(210., 20.), dim);
		font.Draw(it.first, point, grey);
		point += Point(0., 20.);
	}
	
	if(!selectedPilot.empty() && files.find(selectedPilot) != files.end())
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
	Panel *panel = new MainPanel(player);
	gamePanels.Push(panel);
	// Tell the main panel to re-draw itself (and pop up the planet panel).
	panel->Step();
	gamePanels.Push(new ShipyardPanel(player));
}



bool LoadPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == 'n')
	{
		GameData::Revert();
		player.New();
		
		Messages::Reset();
		ConversationPanel *panel = new ConversationPanel(
			player, *GameData::Conversations().Get("intro"));
		GetUI()->Push(panel);
		panel->SetCallback(this, &LoadPanel::OnCallback);
	}
	else if(key == 'd' && !selectedPilot.empty())
	{
		GetUI()->Push(new Dialog(this, &LoadPanel::DeletePilot,
			"Are you sure you want to delete the selected pilot, \""
				+ selectedPilot + "\", and all their saved games?"));
	}
	else if(key == 'a')
	{
		string wasSelected = selectedPilot;
		auto it = files.find(selectedPilot);
		if(it == files.end() || it->second.empty() || it->second.front().size() < 4)
			return false;
		
		// Extract the date from this pilot's most recent save.
		string date = "~0000-00-00.txt";
		string from = Files::Saves() + it->second.front();
		DataFile file(from);
		for(const DataNode &node : file)
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
		Files::Copy(from, to);
		UpdateLists();
		
		selectedPilot = wasSelected;
		selectedFile = to.substr(Files::Saves().size());
		loadedInfo.Load(Files::Saves() + selectedFile);
	}
	else if(key == 'r' && !selectedFile.empty())
	{
		GetUI()->Push(new Dialog(this, &LoadPanel::DeleteSave,
			"Are you sure you want to delete the selected saved game file, \""
				+ selectedFile + "\"?"));
	}
	else if(key == 'e')
	{
		GameData::Revert();
		loadedInfo.ApplyChanges();
		player.Steal(loadedInfo);
		
		Messages::Reset();
		GetUI()->Pop(this);
		GetUI()->Pop(GetUI()->Root().get());
		gamePanels.Reset();
		gamePanels.Push(new MainPanel(player));
	}
	else if(key == 'b' || key == GameData::Keys().Get(Key::MENU))
		GetUI()->Pop(this);
	else if((key == SDLK_DOWN || key == SDLK_UP) && !files.empty())
	{
		auto pit = files.find(selectedPilot);
		if(sideHasFocus)
		{
			auto it = files.begin();
			for( ; it != files.end(); ++it)
				if(it->first == selectedPilot)
					break;
			
			if(key == SDLK_DOWN)
			{
				++it;
				if(it == files.end())
					it = files.begin();
			}
			else
			{
				if(it == files.begin())
					it = files.end();
				--it;
			}
			selectedPilot = it->first;
			selectedFile = it->second.front();
		}
		else if(pit != files.end())
		{
			auto it = pit->second.begin();
			for( ; it != pit->second.end(); ++it)
				if(*it == selectedFile)
					break;
			
			if(key == SDLK_DOWN)
			{
				++it;
				if(it == pit->second.end())
					it = pit->second.begin();
			}
			else
			{
				if(it == pit->second.begin())
					it = pit->second.end();
				--it;
			}
			selectedFile = *it;
		}
		loadedInfo.Load(Files::Saves() + selectedFile);
	}
	else if(key == SDLK_LEFT)
		sideHasFocus = true;
	else if(key == SDLK_RIGHT)
		sideHasFocus = false;
	else
		return false;
	
	return true;
}



bool LoadPanel::Click(int x, int y)
{
	char key = GameData::Interfaces().Get("load menu")->OnClick(Point(x, y));
	if(key != '\0')
		return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	
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
		sideHasFocus = true;
	}
	else if(x >= -100 && x < 100)
	{
		int i = 0;
		for(const string &file : files.find(selectedPilot)->second)
			if(i++ == selected && selectedFile != file)
				selectedFile = file;
		sideHasFocus = false;
	}
	else
		return false;
	
	loadedInfo.Load(Files::Saves() + selectedFile);
	
	return true;
}



void LoadPanel::UpdateLists()
{
	files.clear();
	
	vector<string> fileList = Files::List(Files::Saves());
	for(const string &path : fileList)
	{
		string fileName = path.substr(Files::Saves().size());
		// The file name is either "Pilot Name.txt" or "Pilot Name~Date.txt".
		size_t pos = fileName.find('~');
		if(pos == string::npos)
			pos = fileName.size() - 4;
		
		string pilotName = fileName.substr(0, pos);
		files[pilotName].push_back(fileName);
	}
	
	for(auto &it : files)
		sort(it.second.begin(), it.second.end());
	
	if(!files.empty())
	{
		if(selectedPilot.empty())
			selectedPilot = files.begin()->first;
		if(selectedFile.empty())
		{
			auto it = files.find(selectedPilot);
			if(it != files.end())
			{
				selectedFile =it->second.front();
				loadedInfo.Load(Files::Saves() + selectedFile);
			}
		}
	}
}



void LoadPanel::DeletePilot()
{
	loadedInfo.Clear();
	if(selectedPilot == player.Identifier())
		player.Clear();
	auto it = files.find(selectedPilot);
	if(it == files.end())
		return;
	
	for(const string &file : it->second)
		Files::Delete(Files::Saves() + file);
	
	sideHasFocus = true;
	selectedPilot.clear();
	selectedFile.clear();
	UpdateLists();
}



void LoadPanel::DeleteSave()
{
	loadedInfo.Clear();
	string pilot = selectedPilot;
	Files::Delete(Files::Saves() + selectedFile);
	
	sideHasFocus = true;
	selectedPilot.clear();
	UpdateLists();
	
	auto it = files.find(pilot);
	if(it != files.end() && !it->second.empty())
	{
		selectedFile = it->second.front();
		selectedPilot = pilot;
		loadedInfo.Load(Files::Saves() + selectedFile);
	}
}
