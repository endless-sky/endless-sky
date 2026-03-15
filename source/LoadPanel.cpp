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

#include "text/Alignment.h"
#include "Color.h"
#include "Command.h"
#include "ConversationPanel.h"
#include "DataFile.h"
#include "DialogPanel.h"
#include "text/DisplayText.h"
#include "Files.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "GamerulesPanel.h"
#include "Information.h"
#include "Interface.h"
#include "MainPanel.h"
#include "image/MaskManager.h"
#include "PilotProfile.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "shader/StarField.h"
#include "StartConditionsPanel.h"
#include "text/Truncate.h"
#include "UI.h"

#include "opengl.h"

#include <algorithm>
#include <cstdlib>
#include <ranges>

using namespace std;

namespace {
	// Extract the date from this pilot's most recent save.
	string FileDate(const filesystem::path &filename)
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
}



LoadPanel::LoadPanel(PlayerInfo &player, UI &gamePanels)
	: player(player), gamePanels(gamePanels), selectedPilot(player.Pilot()),
	pilotBox(GameData::Interfaces().Get("load menu")->GetBox("pilots")),
	snapshotBox(GameData::Interfaces().Get("load menu")->GetBox("snapshots")),
	tooltip(200, Alignment::LEFT, Tooltip::Direction::DOWN_LEFT, Tooltip::Corner::TOP_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium"))
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
	GameData::Background().Draw(Point());
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

	if(selectedPilot)
	{
		info.SetCondition("pilot selected");
		if(!selectedPilot->GetGamerules().LockGamerules())
			info.SetCondition("gamerules unlocked");
	}
	if(!player.IsDead() && player.IsLoaded() && selectedPilot)
		info.SetCondition("pilot alive");
	if(selectedFile.find('~') != string::npos)
		info.SetCondition("snapshot selected");
	if(loadedInfo.IsLoaded())
		info.SetCondition("pilot loaded");

	const Interface *loadPanel = GameData::Interfaces().Get("load menu");

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
		for(const auto &[identifier, pilot] : pilots)
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
			bool isHighlighted = (pilot == selectedPilot || (hasHover && zone.Contains(hoverPoint)));

			double alpha = min((drawPoint.Y() - (top - fadeOut)) * .1,
					(bottom - fadeOut - drawPoint.Y()) * .1);
			alpha = max(alpha, 0.);
			alpha = min(alpha, 1.);

			if(pilot == selectedPilot)
				FillShader::Fill(zone, Color(.1 * alpha, 0.));
			const int textWidth = pilotBox.Width() - 2. * hTextPad;
			font.Draw({identifier, {textWidth, Truncate::BACK}}, textPoint, Color((isHighlighted ? .7 : .5) * alpha, 0.));
		}
	}

	// Draw the list of snapshots for the selected pilot.
	bool hasHoverZone = false;
	if(selectedPilot)
	{
		const Point topLeft = snapshotBox.TopLeft();
		Point currentTopLeft = topLeft + Point(0, -centerScroll);
		const double top = topLeft.Y();
		const double bottom = top + snapshotBox.Height();
		const double hTextPad = loadPanel->GetValue("snapshot horizontal text pad");
		const double fadeOut = loadPanel->GetValue("snapshot fade out");
		for(const auto &[file, time] : selectedPilot->Files())
		{
			const Point drawPoint = currentTopLeft;
			currentTopLeft += Point(0., 20.);

			if(drawPoint.Y() < top - fadeOut)
				continue;
			if(drawPoint.Y() > bottom - fadeOut)
				continue;

			Rectangle zone(drawPoint + Point(snapshotBox.Width() / 2., 10.), Point(snapshotBox.Width(), 20.));
			const Point textPoint(drawPoint.X() + hTextPad, zone.Center().Y() - font.Height() / 2);
			bool isHovering = (hasHover && zone.Contains(hoverPoint));
			bool isHighlighted = (file == selectedFile || isHovering);
			if(isHovering)
			{
				hasHoverZone = true;
				tooltip.IncrementCount();
				if(tooltip.ShouldDraw())
				{
					tooltip.SetText(Format::TimestampString(time), true);
					tooltip.SetZone(zone);
				}
			}

			double alpha = min((drawPoint.Y() - (top - fadeOut)) * .1,
					(bottom - fadeOut - drawPoint.Y()) * .1);
			alpha = max(alpha, 0.);
			alpha = min(alpha, 1.);

			if(file == selectedFile)
				FillShader::Fill(zone, Color(.1 * alpha, 0.));
			size_t pos = file.find('~') + 1;
			const string name = file.substr(pos, file.size() - 4 - pos);
			const int textWidth = snapshotBox.Width() - 2. * hTextPad;
			font.Draw({name, {textWidth, Truncate::BACK}}, textPoint, Color((isHighlighted ? .7 : .5) * alpha, 0.));
		}
	}

	if(!hasHoverZone)
		tooltip.DecrementCount();
	else
		tooltip.Draw();
}



void LoadPanel::UpdateTooltipActivation()
{
	tooltip.UpdateActivationCount();
}



bool LoadPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NORMAL;

	if(key == 'n')
	{
		// If no player is loaded, the "Enter Ship" button becomes "New Pilot."
		// Request that the player chooses a start scenario.
		// StartConditionsPanel also handles the case where there's no scenarios.
		GetUI().Push(new StartConditionsPanel(player, gamePanels, GameData::StartOptions(), this));
	}
	else if(key == 'd' && selectedPilot)
	{
		sound = UI::UISound::NONE;
		GetUI().Push(DialogPanel::RequestStringWithValidation(this, &LoadPanel::DeletePilot,
			[this](const string &pilot) { return pilot == loadedInfo.Name(); },
			"Are you sure you want to delete the selected pilot, \"" + loadedInfo.Name()
				+ "\", and all their saved games?\n\n(This will permanently delete the pilot data.)\n"
				+ "Confirm the name of the pilot you want to delete."));
	}
	else if(key == 'a' && !player.IsDead() && player.IsLoaded())
	{
		if(!selectedPilot || selectedPilot->Files().empty() || selectedPilot->Files().front().first.size() < 4)
			return false;

		sound = UI::UISound::NONE;
		nameToConfirm.clear();
		filesystem::path lastSave = Files::Saves() / selectedPilot->Files().front().first;
		GetUI().Push(DialogPanel::RequestString(this, &LoadPanel::SnapshotCallback,
			"Enter a name for this snapshot, or use the most recent save's date:",
			FileDate(lastSave)));
	}
	else if(key == 'R' && !selectedFile.empty())
	{
		sound = UI::UISound::NONE;
		string fileName = selectedFile.substr(selectedFile.rfind('/') + 1);
		if(fileName != selectedPilot->Identifier() + ".txt")
			GetUI().Push(DialogPanel::CallFunctionIfOk(this, &LoadPanel::DeleteSave,
				"Are you sure you want to delete the selected saved game file, \""
					+ selectedFile + "\"?"));
	}
	else if((key == 'l' || key == 'e') && selectedPilot)
	{
		// Is the selected file a snapshot or the pilot's main file?
		string fileName = selectedFile.substr(selectedFile.rfind('/') + 1);
		if(fileName == selectedPilot->Identifier() + ".txt")
			LoadCallback();
		else
		{
			sound = UI::UISound::NONE;
			GetUI().Push(DialogPanel::CallFunctionIfOk(this, &LoadPanel::LoadCallback,
				"If you load this snapshot, it will overwrite your current game. "
				"Any progress will be lost, unless you have saved other snapshots. "
				"Are you sure you want to do that?"));
		}
	}
	else if(key == 'g' && selectedPilot && !selectedPilot->GetGamerules().LockGamerules())
	{
		GamerulesPanel *panel = new GamerulesPanel(selectedPilot->GetGamerules(), true);
		panel->SetCallback(selectedPilot.get(), &PilotProfile::Save);
		GetUI().Push(panel);
	}
	else if(key == 'o')
		Files::OpenUserSavesFolder();
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI().Pop(this);
	else if((key == SDLK_DOWN || key == SDLK_UP) && !pilots.empty())
	{
		auto pit = pilots.find(selectedPilot->Identifier());
		if(sideHasFocus)
		{
			auto it = pilots.begin();
			int index = 0;
			for( ; it != pilots.end(); ++it, ++index)
				if(it->second == selectedPilot)
					break;

			if(key == SDLK_DOWN)
			{
				const int lastVisibleIndex = (sideScroll / 20.) + 13.;
				if(index >= lastVisibleIndex)
					sideScroll += 20.;
				++it;
				if(it == pilots.end())
				{
					it = pilots.begin();
					sideScroll = 0.;
				}
			}
			else
			{
				const int firstVisibleIndex = sideScroll / 20.;
				if(index <= firstVisibleIndex)
					sideScroll -= 20.;
				if(it == pilots.begin())
				{
					it = pilots.end();
					sideScroll = max(0., 20. * pilots.size() - 280.);
				}
				--it;
			}
			selectedPilot = it->second;
			selectedFile = it->second->Files().front().first;
			centerScroll = 0.;
		}
		else if(pit != pilots.end())
		{
			auto &saveFiles = pit->second->Files();
			auto it = saveFiles.begin();
			int index = 0;
			for( ; it != saveFiles.end(); ++it, ++index)
				if(it->first == selectedFile)
					break;

			if(key == SDLK_DOWN)
			{
				++it;
				const int lastVisibleIndex = (centerScroll / 20.) + 13.;
				if(index >= lastVisibleIndex)
					centerScroll += 20.;
				if(it == saveFiles.end())
				{
					it = saveFiles.begin();
					centerScroll = 0.;
				}
			}
			else
			{
				const int firstVisibleIndex = centerScroll / 20.;
				if(index <= firstVisibleIndex)
					centerScroll -= 20.;
				if(it == saveFiles.begin())
				{
					it = saveFiles.end();
					centerScroll = max(0., 20. * saveFiles.size() - 280.);
				}
				--it;
			}
			selectedFile = it->first;
		}
		loadedInfo.Load(Files::Saves() / selectedFile);
	}
	else if(key == SDLK_LEFT)
		sideHasFocus = true;
	else if(key == SDLK_RIGHT)
		sideHasFocus = false;
	else
		return false;

	UI::PlaySound(sound);
	return true;
}



bool LoadPanel::Click(int x, int y, MouseButton button, int clicks)
{
	// When the user clicks, clear the hovered state.
	hasHover = false;
	if(button != MouseButton::LEFT)
		return false;

	const Point click(x, y);

	if(pilotBox.Contains(click))
	{
		int selected = (y + sideScroll - pilotBox.Top()) / 20;
		int i = 0;
		for(const auto &pilot : pilots | views::values)
			if(i++ == selected && selectedPilot != pilot)
			{
				selectedPilot = pilot;
				selectedFile = pilot->Files().front().first;
				centerScroll = 0;
				UI::PlaySound(UI::UISound::NORMAL);
			}
	}
	else if(snapshotBox.Contains(click))
	{
		int selected = (y + centerScroll - snapshotBox.Top()) / 20;
		int i = 0;
		if(!selectedPilot)
			return true;
		for(const auto &file : selectedPilot->Files() | views::keys)
			if(i++ == selected)
			{
				const bool sameSelected = selectedFile == file;
				selectedFile = file;
				if(sameSelected && clicks > 1)
					KeyDown('l', 0, Command(), true);
				break;
			}
	}
	else
		return false;

	if(!selectedFile.empty())
		loadedInfo.Load(Files::Saves() / selectedFile);

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
	if(!tooltip.ShouldDraw())
		tooltip.ResetCount();

	return true;
}



bool LoadPanel::Drag(double dx, double dy)
{
	if(sideHasFocus)
		sideScroll = max(0., min(20. * pilots.size() - 280., sideScroll - dy));
	else if(selectedPilot)
		centerScroll = max(0., min(20. * selectedPilot->Files().size() - 280., centerScroll - dy));
	return true;
}



bool LoadPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



void LoadPanel::UpdateLists()
{
	PilotProfile::LoadProfiles();
	pilots = PilotProfile::GetProfileMap();

	if(!pilots.empty())
	{
		if(!selectedPilot)
			selectedPilot = pilots.begin()->second;
		if(selectedFile.empty())
		{
			if(selectedPilot)
			{
				selectedFile = selectedPilot->Files().front().first;
				loadedInfo.Load(Files::Saves() / selectedFile);
			}
		}
	}
}



// Snapshot name callback.
void LoadPanel::SnapshotCallback(const string &name)
{
	if(!selectedPilot || selectedPilot->Files().empty() || selectedPilot->Files().front().first.size() < 4)
		return;

	filesystem::path from = Files::Saves() / selectedPilot->Files().front().first;
	string suffix = name.empty() ? FileDate(from) : name;
	string extension = "~" + suffix + ".txt";

	// If a file with this name already exists, make sure the player
	// actually wants to overwrite it.
	filesystem::path to = from.parent_path() / (from.stem().string() + extension);
	if(Files::Exists(to) && suffix != nameToConfirm)
	{
		nameToConfirm = suffix;
		GetUI().Push(DialogPanel::RequestString(this, &LoadPanel::SnapshotCallback, "Warning: \"" + suffix
			+ "\" is being used for an existing snapshot.\nOverwrite it?", suffix));
	}
	else
		WriteSnapshot(from, to);
}



// This name is the one to be used, even if it already exists.
void LoadPanel::WriteSnapshot(const filesystem::path &sourceFile, const filesystem::path &snapshotName)
{
	// Copy the autosave to a new, named file.
	if(Files::Copy(sourceFile, snapshotName))
	{
		UpdateLists();
		selectedFile = Files::Name(snapshotName);
		loadedInfo.Load(Files::Saves() / selectedFile);
	}
	else
		GetUI().Push(DialogPanel::Info("Error: unable to create the file \"" + snapshotName.string() + "\"."));
}



// Load snapshot callback.
void LoadPanel::LoadCallback()
{
	// First, make sure the previous MainPanel has been deleted, so
	// its background thread is no longer running.
	gamePanels.Reset();
	gamePanels.CanSave(true);

	player.Load(loadedInfo.Path(), pilots[loadedInfo.Identifier()]);

	// Scale any new masks that might have been added by the newly loaded save file.
	GameData::GetMaskManager().ScaleMasks();

	GetUI().PopThrough(GetUI().Root().get());
	gamePanels.Push(new MainPanel(player));
	// It takes one step to figure out the planet panel should be created, and
	// another step to actually place it. So, take two steps to avoid a flicker.
	gamePanels.StepAll();
	gamePanels.StepAll();
}



void LoadPanel::DeletePilot(const string &)
{
	loadedInfo.Clear();
	if(selectedPilot == player.Pilot())
		player.Clear();
	if(!selectedPilot)
		return;

	bool failed = false;
	for(const auto &file : selectedPilot->Files() | views::keys)
	{
		filesystem::path path = Files::Saves() / file;
		Files::Delete(path);
		failed |= Files::Exists(path);
	}
	if(failed)
		GetUI().Push(DialogPanel::Info("Deleting pilot files failed."));

	sideHasFocus = true;
	selectedPilot.reset();
	selectedFile.clear();
	UpdateLists();
}



void LoadPanel::DeleteSave()
{
	loadedInfo.Clear();
	string pilot = selectedPilot->Identifier();
	filesystem::path path = Files::Saves() / selectedFile;
	Files::Delete(path);
	if(Files::Exists(path))
		GetUI().Push(DialogPanel::Info("Deleting snapshot file failed."));

	sideHasFocus = true;
	selectedPilot.reset();
	UpdateLists();

	auto it = pilots.find(pilot);
	if(it != pilots.end() && !it->second->Files().empty())
	{
		selectedPilot = it->second;
		selectedFile = selectedPilot->Files().front().first;
		loadedInfo.Load(Files::Saves() / selectedFile);
		sideHasFocus = false;
	}
}
