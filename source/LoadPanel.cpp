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
#include "text/DisplayText.h"
#include "Files.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "MainPanel.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "ShipyardPanel.h"
#include "StarField.h"
#include "StartConditionsPanel.h"
#include "text/truncate.hpp"
#include "UI.h"

#include "gl_header.h"

#include <algorithm>
#include <stdexcept>

using namespace std;

namespace {
	// Convert a time_t to a human-readable time and date.
	string TimestampString(time_t timestamp)
	{
		static const size_t BUF_SIZE = 24;
		char buf[BUF_SIZE];
		
		const tm *date = localtime(&timestamp);
#ifdef _WIN32
		static const char *FORMAT = "%#I:%M %p on %#d %b %Y";
#else
		static const char *FORMAT = "%-I:%M %p on %-d %b %Y";
#endif
		return string(buf, strftime(buf, BUF_SIZE, FORMAT, date));
	}
	
	// Extract the date from this pilot's most recent save.
	string FileDate(const string &filename)
	{
		string date = "0000-00-00";
		DataFile file(filename);
		for(const DataNode &node : file)
			if(node.Token(0) == "date")
			{
				int year = node.Value(3);
				int month = node.Value(2);
				int day = node.Value(1);
				date[0] += (year / 1000) % 10;
				date[1] += (year / 100) % 10;
				date[2] += (year / 10) % 10;
				date[3] += year % 10;
				date[5] += (month / 10) % 10;
				date[6] += month % 10;
				date[8] += (day / 10) % 10;
				date[9] += day % 10;
				break;
			}
		return date;
	}
	
	// Only show tooltips if the mouse has hovered in one place for this amount
	// of time.
	const int HOVER_TIME = 60;
}



LoadPanel::LoadPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), selectedPilot(player.Identifier())
{
	// If you have a player loaded, and the player is on a planet, make sure
	// the player is saved so that any snapshot you create will be of the
	// player's current state, rather than one planet ago. Only do this if the
	// game is paused and 'dirty', i.e. the "main panel" is not on top, and we
	// actually were using the loaded save.
	if(player.GetPlanet() && !player.IsDead() && !gamePanels.IsTop(&*gamePanels.Root())
			&& gamePanels.CanSave())
		player.Save();
	UpdateLists();
}



void LoadPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	const Font &font = FontSet::Get(14);
	
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
		info.SetString("playtime", loadedInfo.GetPlayTime());
	}
	else
		info.SetString("pilot", "No Pilot Loaded");
	
	if(!selectedPilot.empty())
		info.SetCondition("pilot selected");
	if(!player.IsDead() && player.IsLoaded() && !selectedPilot.empty())
		info.SetCondition("pilot alive");
	if(selectedFile.find('~') != string::npos)
		info.SetCondition("snapshot selected");
	if(loadedInfo.IsLoaded())
		info.SetCondition("pilot loaded");
	
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("load menu")->Draw(info, this);
	GameData::Interfaces().Get("menu player info")->Draw(info, this);
	
	// The list has space for 14 entries. Alpha should be 100% for Y = -157 to
	// 103, and fade to 0 at 10 pixels beyond that.
	Point point(-470., -157. - sideScroll);
	const double centerY = font.Height() / 2;
	for(const auto &it : files)
	{
		Rectangle zone(point + Point(110., centerY), Point(230., 20.));
		bool isHighlighted = (it.first == selectedPilot || (hasHover && zone.Contains(hoverPoint)));
		
		double alpha = min(1., max(0., min(.1 * (113. - point.Y()), .1 * (point.Y() - -167.))));
		if(it.first == selectedPilot)
			FillShader::Fill(zone.Center(), zone.Dimensions(), Color(.1 * alpha, 0.));
		font.Draw({it.first, {220, Truncate::BACK}}, point, Color((isHighlighted ? .7 : .5) * alpha, 0.));
		point += Point(0., 20.);
	}
	
	// The hover count "decays" over time if not hovering over a saved game.
	if(hoverCount)
		--hoverCount;
	string hoverText;
	
	if(!selectedPilot.empty() && files.count(selectedPilot))
	{
		point = Point(-110., -157. - centerScroll);
		for(const auto &it : files.find(selectedPilot)->second)
		{
			const string &file = it.first;
			Rectangle zone(point + Point(110., centerY), Point(230., 20.));
			double alpha = min(1., max(0., min(.1 * (113. - point.Y()), .1 * (point.Y() - -167.))));
			bool isHovering = (alpha && hasHover && zone.Contains(hoverPoint));
			bool isHighlighted = (file == selectedFile || isHovering);
			if(isHovering)
			{
				hoverCount = min(HOVER_TIME, hoverCount + 2);
				if(hoverCount == HOVER_TIME)
					hoverText = TimestampString(it.second);
			}
			
			if(file == selectedFile)
				FillShader::Fill(zone.Center(), zone.Dimensions(), Color(.1 * alpha, 0.));
			size_t pos = file.find('~') + 1;
			const string name = file.substr(pos, file.size() - 4 - pos);
			font.Draw({name, {220, Truncate::BACK}}, point, Color((isHighlighted ? .7 : .5) * alpha, 0.));
			point += Point(0., 20.);
		}
	}
	if(!hoverText.empty())
	{
		Point boxSize(font.Width(hoverText) + 20., 30.);
		
		FillShader::Fill(hoverPoint + .5 * boxSize, boxSize, *GameData::Colors().Get("tooltip background"));
		font.Draw(hoverText, hoverPoint + Point(10., 10.), *GameData::Colors().Get("medium"));
	}
}



bool LoadPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == 'n')
	{
		// If no player is loaded, the "Enter Ship" button becomes "New Pilot."
		// Request that the player chooses a start scenario.
		// StartConditionsPanel also handles the case where there's no scenarios.
		GetUI()->Push(new StartConditionsPanel(player, gamePanels, GameData::StartOptions(), this));
	}
	else if(key == 'D' && !selectedPilot.empty())
	{
		GetUI()->Push(new Dialog(this, &LoadPanel::DeletePilot,
			"Are you sure you want to delete the selected pilot, \"" + selectedPilot
				+ "\", and all their saved games?\n\n(This will permanently delete the pilot data.)"));
	}
	else if(key == 'a' && !player.IsDead() && player.IsLoaded())
	{
		auto it = files.find(selectedPilot);
		if(it == files.end() || it->second.empty() || it->second.front().first.size() < 4)
			return false;
		
		nameToConfirm.clear();
		string lastSave = Files::Saves() + it->second.front().first;
		GetUI()->Push(new Dialog(this, &LoadPanel::SnapshotCallback,
			"Enter a name for this snapshot, or use the most recent save's date:",
			FileDate(lastSave)));
	}
	else if(key == 'R' && !selectedFile.empty())
	{
		string fileName = selectedFile.substr(selectedFile.rfind('/') + 1);
		if(!(fileName == selectedPilot + ".txt"))
			GetUI()->Push(new Dialog(this, &LoadPanel::DeleteSave,
				"Are you sure you want to delete the selected saved game file, \""
					+ selectedFile + "\"?"));
	}
	else if((key == 'l' || key == 'e') && !selectedPilot.empty())
	{
		// Is the selected file a snapshot or the pilot's main file?
		string fileName = selectedFile.substr(selectedFile.rfind('/') + 1);
		if(fileName == selectedPilot + ".txt")
			LoadCallback();
		else
			GetUI()->Push(new Dialog(this, &LoadPanel::LoadCallback,
				"If you load this snapshot, it will overwrite your current game. "
				"Any progress will be lost, unless you have saved other snapshots. "
				"Are you sure you want to do that?"));
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
			selectedFile = it->second.front().first;
		}
		else if(pit != files.end())
		{
			auto it = pit->second.begin();
			for( ; it != pit->second.end(); ++it)
				if(it->first == selectedFile)
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
			selectedFile = it->first;
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



bool LoadPanel::Click(int x, int y, int clicks)
{
	// When the user clicks, clear the hovered state.
	hasHover = false;
	
	// The first row of each panel is y = -160 to -140.
	if(y < -160 || y >= (-160 + 14 * 20))
		return false;
	
	if(x >= -470 && x < -250)
	{
		int selected = (y + sideScroll - -160) / 20;
		int i = 0;
		for(const auto &it : files)
			if(i++ == selected && selectedPilot != it.first)
			{
				selectedPilot = it.first;
				selectedFile = it.second.front().first;
				centerScroll = 0;
			}
	}
	else if(x >= -110 && x < 110)
	{
		int selected = (y + centerScroll - -160) / 20;
		int i = 0;
		auto filesIt = files.find(selectedPilot);
		if(filesIt == files.end())
			return true;
		for(const auto &it : filesIt->second)
			if(i++ == selected)
			{
				selectedFile = it.first;
				if(clicks > 1)
					KeyDown('l', 0, Command(), true);
				break;
			}
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
	
	hasHover = true;
	hoverPoint = Point(x, y);
	// Tooltips should not pop up unless the mouse stays in one place for the
	// full hover time. Otherwise, every time the user scrubs the mouse over the
	// list, tooltips will appear after one second.
	if(hoverCount < HOVER_TIME)
		hoverCount = 0;
	
	return true;
}



bool LoadPanel::Drag(double dx, double dy)
{
	auto it = files.find(selectedPilot);
	if(sideHasFocus)
		sideScroll = max(0., min(20. * files.size() - 280., sideScroll - dy));
	else if(!selectedPilot.empty() && it != files.end())
		centerScroll = max(0., min(20. * it->second.size() - 280., centerScroll - dy));
	return true;
}



bool LoadPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



void LoadPanel::UpdateLists()
{
	files.clear();
	
	vector<string> fileList = Files::List(Files::Saves());
	for(const string &path : fileList)
	{
		string fileName = Files::Name(path);
		// The file name is either "Pilot Name.txt" or "Pilot Name~SnapshotTitle.txt".
		size_t pos = fileName.find('~');
		if(pos == string::npos)
			pos = fileName.size() - 4;
		
		string pilotName = fileName.substr(0, pos);
		files[pilotName].emplace_back(fileName, Files::Timestamp(path));
	}
	
	for(auto &it : files)
		sort(it.second.begin(), it.second.end(),
			[](const pair<string, time_t> &a, const pair<string, time_t> &b) -> bool
			{
				return a.second > b.second;
			}
		);
	
	if(!files.empty())
	{
		if(selectedPilot.empty())
			selectedPilot = files.begin()->first;
		if(selectedFile.empty())
		{
			auto it = files.find(selectedPilot);
			if(it != files.end())
			{
				selectedFile = it->second.front().first;
				loadedInfo.Load(Files::Saves() + selectedFile);
			}
		}
	}
}



// Snapshot name callback.
void LoadPanel::SnapshotCallback(const string &name)
{
	auto it = files.find(selectedPilot);
	if(it == files.end() || it->second.empty() || it->second.front().first.size() < 4)
		return;
	
	string from = Files::Saves() + it->second.front().first;
	string suffix = name.empty() ? FileDate(from) : name;
	string extension = "~" + suffix + ".txt";
	
	// If a file with this name already exists, make sure the player
	// actually wants to overwrite it.
	string to = from.substr(0, from.size() - 4) + extension;
	if(Files::Exists(to) && suffix != nameToConfirm)
	{
		nameToConfirm = suffix;
		GetUI()->Push(new Dialog(this, &LoadPanel::SnapshotCallback, "Warning: \"" + suffix
			+ "\" is being used for an existing snapshot.\nOverwrite it?", suffix));
	}
	else
		WriteSnapshot(from, to);
}



// This name is the one to be used, even if it already exists.
void LoadPanel::WriteSnapshot(const string &sourceFile, const string &snapshotName)
{
	// Copy the autosave to a new, named file.
	Files::Copy(sourceFile, snapshotName);
	if(Files::Exists(snapshotName))
	{
		UpdateLists();
		selectedFile = Files::Name(snapshotName);
		loadedInfo.Load(Files::Saves() + selectedFile);
	}
	else
		GetUI()->Push(new Dialog("Error: unable to create the file \"" + snapshotName + "\"."));
}



// Load snapshot callback.
void LoadPanel::LoadCallback()
{
	// First, make sure the previous MainPanel has been deleted, so
	// its background thread is no longer running.
	gamePanels.Reset();
	gamePanels.CanSave(true);
	
	player.Load(loadedInfo.Path());
	
	GetUI()->Pop(this);
	GetUI()->Pop(GetUI()->Root().get());
	gamePanels.Push(new MainPanel(player));
	// It takes one step to figure out the planet panel should be created, and
	// another step to actually place it. So, take two steps to avoid a flicker.
	gamePanels.StepAll();
	gamePanels.StepAll();
}



void LoadPanel::DeletePilot()
{
	loadedInfo.Clear();
	if(selectedPilot == player.Identifier())
		player.Clear();
	auto it = files.find(selectedPilot);
	if(it == files.end())
		return;
	
	bool failed = false;
	for(const auto &fit : it->second)
	{
		string path = Files::Saves() + fit.first;
		Files::Delete(path);
		failed |= Files::Exists(path);
	}
	if(failed)
		GetUI()->Push(new Dialog("Deleting pilot files failed."));
	
	sideHasFocus = true;
	selectedPilot.clear();
	selectedFile.clear();
	UpdateLists();
}



void LoadPanel::DeleteSave()
{
	loadedInfo.Clear();
	string pilot = selectedPilot;
	string path = Files::Saves() + selectedFile;
	Files::Delete(path);
	if(Files::Exists(path))
		GetUI()->Push(new Dialog("Deleting snapshot file failed."));
	
	sideHasFocus = true;
	selectedPilot.clear();
	UpdateLists();
	
	auto it = files.find(pilot);
	if(it != files.end() && !it->second.empty())
	{
		selectedFile = it->second.front().first;
		selectedPilot = pilot;
		loadedInfo.Load(Files::Saves() + selectedFile);
		sideHasFocus = false;
	}
}
