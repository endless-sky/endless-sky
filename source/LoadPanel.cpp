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
#include "Command.h"
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
#include "PlayerInfo.h"
#include "ShipyardPanel.h"
#include "StarField.h"
#include "UI.h"

#include <algorithm>

using namespace std;



LoadPanel::LoadPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), selectedPilot(player.Identifier())
{
	// If you have a player loaded, and the player is on a planet, makes sure
	// the player is saved so that any snapshot you create will be of the
	// player's current state, rather than one planet ago. Only do this if the
	// game is paused, i.e. the "main panel" is not on top:
	if(player.GetPlanet() && !player.IsDead() && !gamePanels.IsTop(&*gamePanels.Root()))
		player.Save();
	UpdateLists();
}



void LoadPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	
	Information info;
	if(loadedInfo.IsLoaded())
	{
		info.SetString("pilot", loadedInfo.Name());
		if(loadedInfo.ShipSprite())
		{
			info.SetSprite("ship sprite", loadedInfo.ShipSprite());
			info.SetString("ship", loadedInfo.ShipName());
		}
		if(!loadedInfo.GetSystem().empty())
			info.SetString("system", loadedInfo.GetSystem());
		if(!loadedInfo.GetPlanet().empty())
			info.SetString("planet", loadedInfo.GetPlanet());
		info.SetString("credits", loadedInfo.Credits());
		info.SetString("date", loadedInfo.GetDate());
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
	
	const Font &font = FontSet::Get(14);
	
	// The list has space for 14 entries. Alpha should be 100% for Y = -157 to
	// 103, and fade to 0 at 10 pixels beyond that.
	Point point(-470., -157. - sideScroll);
	for(const auto &it : files)
	{
		double alpha = min(1., max(0., min(.1 * (113. - point.Y()), .1 * (point.Y() - -167.))));
		if(it.first == selectedPilot)
			FillShader::Fill(point + Point(110., 7.), Point(230., 20.), Color(.1 * alpha, 0.));
		font.Draw(it.first, point, Color(.5 * alpha, 0.));
		point += Point(0., 20.);
	}
	
	if(!selectedPilot.empty() && files.find(selectedPilot) != files.end())
	{
		point = Point(-110., -157. - centerScroll);
		for(const string &file : files.find(selectedPilot)->second)
		{
			double alpha = min(1., max(0., min(.1 * (113. - point.Y()), .1 * (point.Y() - -167.))));
			if(file == selectedFile)
				FillShader::Fill(point + Point(110., 7.), Point(230., 20.), Color(.1 * alpha, 0.));
			size_t pos = file.find('~') + 1;
			string name = file.substr(pos, file.size() - 4 - pos);
			font.Draw(name, point, Color(.5 * alpha, 0.));
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



// Snapshot name callback.
void LoadPanel::SnapshotCallback(const std::string &name)
{
	string wasSelected = selectedPilot;
	auto it = files.find(selectedPilot);
	if(it == files.end() || it->second.empty() || it->second.front().size() < 4)
		return;
	
	string from = Files::Saves() + it->second.front();
	string extension = "~" + name + ".txt";
	if(name.empty())
	{
		// Extract the date from this pilot's most recent save.
		extension = "~0000-00-00.txt";
		DataFile file(from);
		for(const DataNode &node : file)
			if(node.Token(0) == "date")
			{
				int year = node.Value(3);
				int month = node.Value(2);
				int day = node.Value(1);
				extension[1] += (year / 1000) % 10;
				extension[2] += (year / 100) % 10;
				extension[3] += (year / 10) % 10;
				extension[4] += year % 10;
				extension[6] += (month / 10) % 10;
				extension[7] += month % 10;
				extension[9] += (day / 10) % 10;
				extension[10] += day % 10;
			}
	}
	
	// Copy the autosave to a new, named file.
	string to = from.substr(0, from.size() - 4) + extension;
	Files::Copy(from, to);
	UpdateLists();
	
	selectedPilot = wasSelected;
	selectedFile = Files::Name(to);
	loadedInfo.Load(Files::Saves() + selectedFile);
}



bool LoadPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
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
		
		GetUI()->Push(new Dialog(this, &LoadPanel::SnapshotCallback,
			"Enter a name for this snapshot, or leave the name empty to use the current date:"));
	}
	else if(key == 'r' && !selectedFile.empty())
	{
		GetUI()->Push(new Dialog(this, &LoadPanel::DeleteSave,
			"Are you sure you want to delete the selected saved game file, \""
				+ selectedFile + "\"?"));
	}
	else if(key == 'l')
	{
		// First, make sure the previous MainPanel has been deleted, so
		// its background thread is no longer running.
		gamePanels.Reset();
		
		GameData::Revert();
		player.Load(loadedInfo.Path());
		player.ApplyChanges();
		
		Messages::Reset();
		GetUI()->Pop(this);
		GetUI()->Pop(GetUI()->Root().get());
		gamePanels.Push(new MainPanel(player));
	}
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
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
	if(key)
		return DoKey(key);
	
	// The first row of each panel is y = -160 to -140.
	if(y < -160)
		return false;
	
	if(x >= -470 && x < -250)
	{
		int selected = (y + sideScroll - -160) / 20;
		int i = 0;
		for(const auto &it : files)
			if(i++ == selected && selectedPilot != it.first)
			{
				selectedPilot = it.first;
				selectedFile = it.second.front();
			}
	}
	else if(x >= -110 && x < 110)
	{
		int selected = (y + centerScroll - -160) / 20;
		int i = 0;
		auto filesIt = files.find(selectedPilot);
		if(filesIt == files.end())
			return true;
		for(const string &file : filesIt->second)
			if(i++ == selected && selectedFile != file)
				selectedFile = file;
	}
	else
		return false;
	
	if(!selectedFile.empty())
		loadedInfo.Load(Files::Saves() + selectedFile);
	
	return true;
}



bool LoadPanel::Hover(int x, int y)
{
	if(x >= -470 && x < -250)
		sideHasFocus = true;
	else if(x >= -110 && x < 110)
		sideHasFocus = false;
	
	return true;
}



bool LoadPanel::Drag(int dx, int dy)
{
	auto it = files.find(selectedPilot);
	if(sideHasFocus)
		sideScroll = max(0, min(static_cast<int>(20 * files.size()) - 280, sideScroll - dy));
	else if(!selectedPilot.empty() && it != files.end())
		centerScroll = max(0, min(static_cast<int>(20 * it->second.size()) - 280, centerScroll - dy));
	return true;
}



bool LoadPanel::Scroll(int dx, int dy)
{
	return Drag(50 * dx, 50 * dy);
}



void LoadPanel::UpdateLists()
{
	files.clear();
	
	vector<string> fileList = Files::List(Files::Saves());
	for(const string &path : fileList)
	{
		string fileName = Files::Name(path);
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
