/* LoadPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
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
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "MainPanel.h"
#include "image/MaskManager.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "StarField.h"
#include "StartConditionsPanel.h"
#include "text/truncate.hpp"
#include "UI.h"

#ifdef __ANDROID__
#include "AndroidFile.h"
#endif

#include "opengl.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>
#include <sstream>

using namespace std;

namespace {
	// Return a pair containing settings to use for time formatting.
	pair<const char*, const char*> TimestampFormatString(Preferences::DateFormat format)
	{
		// pair<string, string>: Linux (1st) and Windows (2nd) format strings.
		switch(format)
		{
			case Preferences::DateFormat::YMD:
				return make_pair("%F %T", "%F %T");
			case Preferences::DateFormat::MDY:
				return make_pair("%-I:%M %p on %b %-d, %Y", "%#I:%M %p on %b %#d, %Y");
			case Preferences::DateFormat::DMY:
			default:
				return make_pair("%-I:%M %p on %-d %b %Y", "%#I:%M %p on %#d %b %Y");
		}
	}

	// Convert a time_t to a human-readable time and date.
	string TimestampString(time_t timestamp)
	{
		pair<const char*, const char*> format = TimestampFormatString(Preferences::GetDateFormat());
		static const size_t BUF_SIZE = 25;
		char str[BUF_SIZE];

#ifdef _WIN32
		tm date;
		localtime_s(&date, &timestamp);
		return string(str, std::strftime(str, BUF_SIZE, format.second, &date));
#else
		const tm *date = localtime(&timestamp);
		return string(str, std::strftime(str, BUF_SIZE, format.first, date));
#endif
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
	: player(player), gamePanels(gamePanels), selectedPilot(player.Identifier()),
	pilotBox(GameData::Interfaces().Get("load menu")->GetBox("pilots")),
	snapshotBox(GameData::Interfaces().Get("load menu")->GetBox("snapshots"))
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

	const Interface *loadPanel = GameData::Interfaces().Get("load menu");

#ifdef ENDLESS_SKY_VERSION
	info.SetString("game version", ENDLESS_SKY_VERSION);
#else
	info.SetString("game version", "engineering build");
#endif

	GameData::Interfaces().Get("menu background")->Draw(info, this);
	loadPanel->Draw(info, this);
	GameData::Interfaces().Get("menu player info")->Draw(info, this);

	// Draw the list of pilots.
	{
		// The list has space for 14 entries. Alpha should be 100% for Y = -157 to
		// 103, and fade to 0 at 10 pixels beyond that.
		const Point topLeft = pilotBox.TopLeft();
		Point currentTopLeft = topLeft + Point(0, -sideScroll);
		const double top = topLeft.Y();
		const double bottom = top + pilotBox.Height();
		const double hTextPad = loadPanel->GetValue("pilot horizontal text pad");
		const double fadeOut = loadPanel->GetValue("pilot fade out");
		for(const auto &it : files)
		{
			const Point drawPoint = currentTopLeft;
			currentTopLeft += Point(0., 20.);

			if(drawPoint.Y() < top - fadeOut)
				continue;
			if(drawPoint.Y() > bottom - fadeOut)
				continue;

			const double width = pilotBox.Width();
			Rectangle zone(drawPoint + Point(width / 2., 10.), Point(width, 20.));
			const Point textPoint(drawPoint.X() + hTextPad, zone.Center().Y() - font.Height() / 2);
			bool isHighlighted = (it.first == selectedPilot || (hasHover && zone.Contains(hoverPoint)));

			double alpha = min((drawPoint.Y() - (top - fadeOut)) * .1,
					(bottom - fadeOut - drawPoint.Y()) * .1);
			alpha = max(alpha, 0.);
			alpha = min(alpha, 1.);

			if(it.first == selectedPilot)
				FillShader::Fill(zone.Center(), zone.Dimensions(), Color(.1 * alpha, 0.));
			const int textWidth = pilotBox.Width() - 2. * hTextPad;
			font.Draw({it.first, {textWidth, Truncate::BACK}}, textPoint, Color((isHighlighted ? .7 : .5) * alpha, 0.));
			if (alpha > 0)
				AddZone(zone, [this, zone]() { Click(zone.Center().X(), zone.Center().Y(), 1); });
		}
	}

	// The hover count "decays" over time if not hovering over a saved game.
	if(hoverCount)
		--hoverCount;
	string hoverText;

	// Draw the list of snapshots for the selected pilot.
	if(!selectedPilot.empty() && files.contains(selectedPilot))
	{
		const Point topLeft = snapshotBox.TopLeft();
		Point currentTopLeft = topLeft + Point(0, -centerScroll);
		const double top = topLeft.Y();
		const double bottom = top + snapshotBox.Height();
		const double hTextPad = loadPanel->GetValue("snapshot horizontal text pad");
		const double fadeOut = loadPanel->GetValue("snapshot fade out");
		for(const auto &it : files.find(selectedPilot)->second)
		{
			const Point drawPoint = currentTopLeft;
			currentTopLeft += Point(0., 20.);

			if(drawPoint.Y() < top - fadeOut)
				continue;
			if(drawPoint.Y() > bottom - fadeOut)
				continue;

			const string &file = it.first;
			Rectangle zone(drawPoint + Point(snapshotBox.Width() / 2., 10.), Point(snapshotBox.Width(), 20.));
			const Point textPoint(drawPoint.X() + hTextPad, zone.Center().Y() - font.Height() / 2);
			bool isHovering = (hasHover && zone.Contains(hoverPoint));
			bool isHighlighted = (file == selectedFile || isHovering);
			if(isHovering)
			{
				hoverCount = min(HOVER_TIME, hoverCount + 2);
				if(hoverCount == HOVER_TIME)
					hoverText = TimestampString(it.second);
			}

			double alpha = min((drawPoint.Y() - (top - fadeOut)) * .1,
					(bottom - fadeOut - drawPoint.Y()) * .1);
			alpha = max(alpha, 0.);
			alpha = min(alpha, 1.);

			if(file == selectedFile)
				FillShader::Fill(zone.Center(), zone.Dimensions(), Color(.1 * alpha, 0.));
			size_t pos = file.find('~') + 1;
			const string name = file.substr(pos, file.size() - 4 - pos);
			const int textWidth = snapshotBox.Width() - 2. * hTextPad;
			font.Draw({name, {textWidth, Truncate::BACK}}, textPoint, Color((isHighlighted ? .7 : .5) * alpha, 0.));
			if (alpha > 0)
				AddZone(zone, [this, zone]() { Click(zone.Center().X(), zone.Center().Y(), 1); });
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
	else if(key == 'd' && !selectedPilot.empty())
	{
		GetUI()->Push(new Dialog(this, &LoadPanel::DeletePilot,
			"Are you sure you want to delete the selected pilot, \"" + loadedInfo.Name()
				+ "\", and all their saved games?\n\n(This will permanently delete the pilot data.)\n"
				+ "Confirm the name of the pilot you want to delete.",
				[this](const string &pilot) { return pilot == loadedInfo.Name(); }));
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
#ifdef __ANDROID__
	else if ((key == 'x') && !selectedPilot.empty())
	{
		// export the pilot
		SDL_Log("Export");
		std::string path = Files::Saves() + selectedFile;
		std::string data = Files::Read(path);

		AndroidFile f;
		f.SaveFile(selectedPilot + "_exported.txt", data);
	}
	else if (key == 'i')
	{
		// import a pilot
		SDL_Log("Import");
		AndroidFile f;
		std::string data = f.GetFile("Import save file");

		if (!data.empty())
		{
			// Validate the file, and load the pilot name
			std::stringstream data_in(data);
			DataFile imported(data_in);
			// DataFile won't fail, even if being fed arbitrary garbage. Validate
			// this file by checking for some specific keys
			std::set<std::string> expected_keys{"pilot","date","system","planet","playtime"};
			std::string pilot_name;
			for (auto& node: imported)
			{
				expected_keys.erase(node.Token(0));
				if (node.Token(0) == "pilot")
				{
					pilot_name = node.Token(1) + " " + node.Token(2);
				}
			}
			if (expected_keys.empty())
			{
				// looks good. lets write it.
				std::string path = Files::Saves() + pilot_name + "~imported.txt";
				int idx = 1;
				while (Files::Exists(path))
				{
					path = Files::Saves() + pilot_name + "~imported-"
												+ std::to_string(idx) + ".txt";
					++idx;
				}
				Files::Write(path, data);
				UpdateLists();
			}
			else
			{
				GetUI()->Push(new Dialog(
					"The selected file does not appear to be a valid "
					"endless-sky save game."));
			}
		}
	}
#endif
	else if(key == 'b' || command.Has(Command::MENU) || key == SDLK_AC_BACK || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if((key == SDLK_DOWN || key == SDLK_UP) && !files.empty())
	{
		auto pit = files.find(selectedPilot);
		if(sideHasFocus)
		{
			auto it = files.begin();
			int index = 0;
			for( ; it != files.end(); ++it, ++index)
				if(it->first == selectedPilot)
					break;

			if(key == SDLK_DOWN)
			{
				const int lastVisibleIndex = (sideScroll / 20.) + 13.;
				if(index >= lastVisibleIndex)
					sideScroll += 20.;
				++it;
				if(it == files.end())
				{
					it = files.begin();
					sideScroll = 0.;
				}
			}
			else
			{
				const int firstVisibleIndex = sideScroll / 20.;
				if(index <= firstVisibleIndex)
					sideScroll -= 20.;
				if(it == files.begin())
				{
					it = files.end();
					sideScroll = max(0., 20. * files.size() - 280.);
				}
				--it;
			}
			selectedPilot = it->first;
			selectedFile = it->second.front().first;
			centerScroll = 0.;
		}
		else if(pit != files.end())
		{
			auto it = pit->second.begin();
			int index = 0;
			for( ; it != pit->second.end(); ++it, ++index)
				if(it->first == selectedFile)
					break;

			if(key == SDLK_DOWN)
			{
				++it;
				const int lastVisibleIndex = (centerScroll / 20.) + 13.;
				if(index >= lastVisibleIndex)
					centerScroll += 20.;
				if(it == pit->second.end())
				{
					it = pit->second.begin();
					centerScroll = 0.;
				}
			}
			else
			{
				const int firstVisibleIndex = centerScroll / 20.;
				if(index <= firstVisibleIndex)
					centerScroll -= 20.;
				if(it == pit->second.begin())
				{
					it = pit->second.end();
					centerScroll = max(0., 20. * pit->second.size() - 280.);
				}
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

	const Point click(x, y);

	if(pilotBox.Contains(click))
	{
		int selected = (y + sideScroll - pilotBox.Top()) / 20;
		int i = 0;
		for(const auto &it : files)
			if(i++ == selected && selectedPilot != it.first)
			{
				selectedPilot = it.first;
				selectedFile = it.second.front().first;
				centerScroll = 0;
			}
	}
	else if(snapshotBox.Contains(click))
	{
		int selected = (y + centerScroll - snapshotBox.Top()) / 20;
		int i = 0;
		auto filesIt = files.find(selectedPilot);
		if(filesIt == files.end())
			return true;
		for(const auto &it : filesIt->second)
			if(i++ == selected)
			{
				const bool sameSelected = selectedFile == it.first;
				selectedFile = it.first;
				if(sameSelected && clicks > 1)
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
	if(x >= pilotBox.Left() && x < pilotBox.Right())
		sideHasFocus = true;
	else if(x >= snapshotBox.Left() && x < snapshotBox.Right())
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
		// Skip any files that aren't text files.
		if(path.compare(path.length() - 4, 4, ".txt"))
			continue;

		string fileName = Files::Name(path);
		// The file name is either "Pilot Name.txt" or "Pilot Name~SnapshotTitle.txt".
		size_t pos = fileName.find('~');
		const bool isSnapshot = (pos != string::npos);
		if(!isSnapshot)
			pos = fileName.size() - 4;

		string pilotName = fileName.substr(0, pos);
		auto &savesList = files[pilotName];
		savesList.emplace_back(fileName, Files::Timestamp(path));
		// Ensure that the main save for this pilot, not a snapshot, is first in the list.
		if(!isSnapshot)
			swap(savesList.front(), savesList.back());
	}

	for(auto &it : files)
	{
		// Don't include the first item in the sort if this pilot has a non-snapshot save.
		auto start = it.second.begin();
		if(start->first.find('~') == string::npos)
			++start;
		sort(start, it.second.end(),
			[](const pair<string, time_t> &a, const pair<string, time_t> &b) -> bool
			{
				return a.second > b.second || (a.second == b.second && a.first < b.first);
			}
		);
	}

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

	// Scale any new masks that might have been added by the newly loaded save file.
	GameData::GetMaskManager().ScaleMasks();

	GetUI()->PopThrough(GetUI()->Root().get());
	gamePanels.Push(new MainPanel(player));
	// It takes one step to figure out the planet panel should be created, and
	// another step to actually place it. So, take two steps to avoid a flicker.
	gamePanels.StepAll();
	gamePanels.StepAll();
}



void LoadPanel::DeletePilot(const string &)
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
