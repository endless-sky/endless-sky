/* MissionPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/



#include "MissionPanel.h"

#include "Command.h"
#include "Dialog.h"
#include "DotShader.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "Mission.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "UI.h"

#include <sstream>

using namespace std;

namespace {
	static const int SIDE_WIDTH = 280;
}



MissionPanel::MissionPanel(PlayerInfo &player)
	: MapPanel(player, -4),
	available(player.AvailableJobs()),
	accepted(player.Missions()),
	availableIt(player.AvailableJobs().begin()),
	acceptedIt(player.AvailableJobs().empty() ? accepted.begin() : accepted.end()),
	availableScroll(0), acceptedScroll(0), dragSide(0)
{
	while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
		++acceptedIt;
	
	// Center the system slightly above the center of the screen because the
	// lower panel is taking up more space than the upper one.
	center = Point(0., -80.) - selectedSystem->Position();
	
	wrap.SetWrapWidth(380);
	wrap.SetFont(FontSet::Get(14));
	wrap.SetAlignment(WrappedText::JUSTIFIED);
}



MissionPanel::MissionPanel(const MapPanel &panel)
	: MapPanel(panel),
	available(player.AvailableJobs()),
	accepted(player.Missions()),
	availableIt(player.AvailableJobs().begin()),
	acceptedIt(player.AvailableJobs().empty() ? accepted.begin() : accepted.end()),
	availableScroll(0), acceptedScroll(0), dragSide(0)
{
	while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
		++acceptedIt;
	
	wrap.SetWrapWidth(380);
	wrap.SetFont(FontSet::Get(14));
	wrap.SetAlignment(WrappedText::JUSTIFIED);
}



void MissionPanel::Step()
{
	if(!Preferences::Has("help: jobs"))
	{
		Preferences::Set("help: jobs");
		GetUI()->Push(new Dialog(
			"Taking on jobs is a safe way to earn money. "
			"Special missions are offered in the Space Port; "
			"more mundane jobs are posted on the Job Board. "
			"Most special missions are only offered once: "
			"if you turn one down, it will not be offered to you again.\n"
			"\tThe payment for a job depends on how far you must travel and how much you are carrying. "
			"Jobs that have a time limit pay extra, but you are paid nothing if you miss the deadline.\n"
			"\tAs you gain a combat reputation, new jobs will become available, "
			"including escorting convoys and bounty hunting."));
	}
}



void MissionPanel::Draw() const
{
	MapPanel::Draw();
	
	DrawSelectedSystem();
	Point pos = DrawPanel(
		Screen::TopLeft() + Point(0., -availableScroll),
		"Missions available here:",
		available.size());
	DrawList(available, pos);
	
	pos = DrawPanel(
		Screen::TopRight() + Point(-SIDE_WIDTH, -acceptedScroll),
		"Your current missions:",
		AcceptedVisible());
	DrawList(accepted, pos);
	
	DrawMissionInfo();
	
	Color availableColor(.5, .4, 0., .5);
	Color unavailableColor(.3, .1, 0., .5);
	Color currentColor(0., .5, 0., .5);
	if(availableIt != available.end() && availableIt->Destination())
		DotShader::Draw(availableIt->Destination()->GetSystem()->Position() + center,
			22., 20.5, CanAccept() ? availableColor : unavailableColor);
	if(acceptedIt != accepted.end() && acceptedIt->Destination())
		DotShader::Draw(acceptedIt->Destination()->GetSystem()->Position() + center,
			22., 20.5, currentColor);
	
	// Draw the buttons to switch to other map modes.
	Information info;
	info.SetCondition("is missions");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info);
}



// Only override the ones you need; the default action is to return false.
bool MissionPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == 'd' || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		GetUI()->Pop(this);
		return true;
	}
	else if(key == 'a')
	{
		if(CanAccept())
			Accept();
		else if(acceptedIt != accepted.end())
		{
			GetUI()->Push(new Dialog(this, &MissionPanel::AbortMission,
				"Abort mission \"" + acceptedIt->Name() + "\"?"));
		}
		return true;
	}
	else if(key == 'p' || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapDetailPanel(*this));
	}
	else if(key == 'o')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapOutfitterPanel(*this));
	}
	else if(key == 's')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapShipyardPanel(*this));
	}
	else if(key == SDLK_LEFT && availableIt == available.end())
	{
		acceptedIt = accepted.end();
		availableIt = available.begin();
	}
	else if(key == SDLK_RIGHT && acceptedIt == accepted.end())
	{
		availableIt = available.end();
		acceptedIt = accepted.begin();
		while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
			++acceptedIt;
	}
	else if(key == SDLK_UP)
	{
		if(availableIt != available.end())
		{
			if(availableIt == available.begin())
				availableIt = available.end();
			--availableIt;
		}
		else if(acceptedIt != accepted.end())
		{
			do {
				if(acceptedIt == accepted.begin())
					acceptedIt = accepted.end();
				--acceptedIt;
			} while(!acceptedIt->IsVisible());
		}
	}
	else if(key == SDLK_DOWN)
	{
		if(availableIt != available.end())
		{
			++availableIt;
			if(availableIt == available.end())
				availableIt = available.begin();
		}
		else if(acceptedIt != accepted.end())
		{
			do {
				++acceptedIt;
				if(acceptedIt == accepted.end())
					acceptedIt = accepted.begin();
			} while(!acceptedIt->IsVisible());
		}
	}
	else if(command.Has(Command::MAP))
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapDetailPanel(*this));
		return true;
	}
	else if(key == 'f')
		GetUI()->Push(new Dialog(
			this, &MissionPanel::DoFind, "Search for:"));
	else
		return false;
	
	if(availableIt != available.end())
		Select(availableIt->Destination()->GetSystem());
	else if(acceptedIt != accepted.end())
		Select(acceptedIt->Destination()->GetSystem());
	if(selectedSystem)
		center = Point(0., -80.) - selectedSystem->Position();
	
	return true;
}



bool MissionPanel::Click(int x, int y)
{
	dragSide = 0;
	
	// Handle clicks on the interface buttons.
	{
		const Interface *interface = GameData::Interfaces().Get("mission");
		char key = interface->OnClick(Point(x, y));
		if(key)
			return DoKey(key);
	}
	{
		const Interface *interface = GameData::Interfaces().Get("map buttons");
		char key = interface->OnClick(Point(x, y));
		// In the mission panel, the "Done" button in the button bar should be
		// ignored (and is not shown).
		if(key && key != 'd')
			return DoKey(key);
	}
	
	if(x > Screen::Right() - 80 && y > Screen::Bottom() - 50)
		return DoKey('p');
	
	if(x < Screen::Left() + SIDE_WIDTH)
	{
		unsigned index = max(0, (y + availableScroll - 36 - Screen::Top()) / 20);
		if(index < available.size())
		{
			availableIt = available.begin();
			while(index--)
				++availableIt;
			acceptedIt = accepted.end();
			dragSide = -1;
			Select(availableIt->Destination()->GetSystem());
			center = Point(0., -80.) - selectedSystem->Position();
			return true;
		}
	}
	else if(x >= Screen::Right() - SIDE_WIDTH)
	{
		int index = max(0, (y + acceptedScroll - 36 - Screen::Top()) / 20);
		if(index < AcceptedVisible())
		{
			acceptedIt = accepted.begin();
			while(index || !acceptedIt->IsVisible())
			{
				index -= acceptedIt->IsVisible();
				++acceptedIt;
			}
			availableIt = available.end();
			dragSide = 1;
			Select(acceptedIt->Destination()->GetSystem());
			center = Point(0., -80.) - selectedSystem->Position();
			return true;
		}
	}
	
	// Figure out if a system was clicked on.
	Point click = Point(x, y) - center;
	const System *system = nullptr;
	for(const auto &it : GameData::Systems())
		if(click.Distance(it.second.Position()) < 10.
				&& (player.HasSeen(&it.second) || &it.second == specialSystem))
		{
			system = &it.second;
			break;
		}
	if(system)
	{
		Select(system);
		int options = available.size() + accepted.size();
		while(options--)
		{
			if(availableIt != available.end())
			{
				++availableIt;
				if(availableIt == available.end())
				{
					if(!accepted.empty())
						acceptedIt = accepted.begin();
					else
						availableIt = available.begin();
				}
			}
			else if(acceptedIt != accepted.end())
			{
				++acceptedIt;
				if(acceptedIt == accepted.end())
				{
					if(!available.empty())
						availableIt = available.begin();
					else
						acceptedIt = accepted.begin();
				}
			}
			if(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
				continue;
			
			if(availableIt != available.end() && availableIt->Destination()->GetSystem() == system)
				break;
			if(acceptedIt != accepted.end() && acceptedIt->Destination()->GetSystem() == system)
				break;
		}
	}
	
	return true;
}



bool MissionPanel::Drag(int dx, int dy)
{
	if(dragSide < 0)
	{
		availableScroll = max(0,
			min(static_cast<int>(available.size() * 20 + 70 - Screen::Height()),
				availableScroll - dy));
	}
	else if(dragSide > 0)
	{
		acceptedScroll = max(0,
			min(static_cast<int>(accepted.size() * 20 + 70 - Screen::Height()),
				acceptedScroll - dy));
	}
	else
		MapPanel::Drag(dx, dy);
	
	return true;
}



void MissionPanel::DoFind(const string &text)
{
	Find(text);
}



void MissionPanel::DrawSelectedSystem() const
{
	const Sprite *sprite = SpriteSet::Get("ui/selected system");
	SpriteShader::Draw(sprite, Point(0., Screen::Top() + .5 * sprite->Height()));
	
	string text;
	if(!selectedSystem)
		text = "Selected system: none";
	else if(!player.KnowsName(selectedSystem))
		text = "Selected system: unexplored system";
	else	
		text = "Selected system: " + selectedSystem->Name();
	
	int jumps = distance.Distance(selectedSystem);
	if(jumps == 1)
		text += " (1 jump away)";
	else if(jumps > 0)
		text += " (" + to_string(jumps) + " jumps away)";
	
	const Font &font = FontSet::Get(14);
	Point pos(-.5 * font.Width(text), Screen::Top() + .5 * (30. - font.Height()));
	font.Draw(text, pos, *GameData::Colors().Get("bright"));
}



Point MissionPanel::DrawPanel(Point pos, const string &label, int entries) const
{
	const Font &font = FontSet::Get(14);
	Color back(.125, 1.);
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	
	// Draw the panel.
	Point size(SIDE_WIDTH, 20 * entries + 40);
	FillShader::Fill(pos + .5 * size, size, back);
	
	// Edges:
	const Sprite *bottom = SpriteSet::Get("ui/bottom edge");
	Point edgePos = pos + Point(.5 * size.X(), size.Y());
	Point bottomOff(0., .5 * bottom->Height());
	SpriteShader::Draw(bottom, edgePos + bottomOff);
	
	const Sprite *left = SpriteSet::Get("ui/left edge");
	const Sprite *right = SpriteSet::Get("ui/right edge");
	double dy = .5 * left->Height();
	Point leftOff(-.5 * (size.X() + left->Width()), 0.);
	Point rightOff(.5 * (size.X() + right->Width()), 0.);
	while(dy && edgePos.Y() > Screen::Top())
	{
		edgePos.Y() -= dy;
		SpriteShader::Draw(left, edgePos + leftOff);
		SpriteShader::Draw(right, edgePos + rightOff);
		edgePos.Y() -= dy;
	}
	
	pos += Point(10., 10. + (20. - font.Height()) * .5);
	font.Draw(label, pos, selected);
	FillShader::Fill(
		pos + Point(.5 * size.X() - 5., 15.),
		Point(size.X() - 10., 1.),
		unselected);
	pos.Y() += 5.;
	
	return pos;
}



Point MissionPanel::DrawList(const list<Mission> &list, Point pos) const
{
	const Font &font = FontSet::Get(14);
	Color highlight = *GameData::Colors().Get("faint");
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	Color dim = *GameData::Colors().Get("dim");
	
	for(auto it = list.begin(); it != list.end(); ++it)
	{
		if(!it->IsVisible())
			continue;
		
		pos.Y() += 20.;
		
		bool isSelected = (it == availableIt || it == acceptedIt);
		if(isSelected)
			FillShader::Fill(
				pos + Point(.5 * SIDE_WIDTH - 5., 8.),
				Point(SIDE_WIDTH - 10., 20.),
				highlight);
		
		bool canAccept = (&list != &available || it->HasSpace(player));
		font.Draw(it->Name(), pos,
			(!canAccept ? dim : isSelected ? selected : unselected));
	}
	
	return pos;
}



void MissionPanel::DrawMissionInfo() const
{
	Information info;
	
	// The "accept / abort" button text and activation depends on what mission,
	// if any, is selected, and whether missions are available.
	if(CanAccept())
		info.SetCondition("can accept");
	else if(acceptedIt != accepted.end())
		info.SetCondition("can abort");
	else if(available.size())
		info.SetCondition("cannot accept");
	else
		info.SetCondition("cannot abort");
	
	int cargoFree = -player.Cargo().Used();
	int bunksFree = -player.Cargo().Passengers();
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		cargoFree += ship->Attributes().Get("cargo space") - ship->Cargo().Used();
		bunksFree += ship->Attributes().Get("bunks") - ship->Crew() - ship->Cargo().Passengers();
	}
	info.SetString("cargo free", to_string(cargoFree) + " tons");
	info.SetString("bunks free", to_string(bunksFree) + " bunks");
	
	info.SetString("today", player.GetDate().ToString());
	
	const Interface *interface = GameData::Interfaces().Get("mission");
	interface->Draw(info);
	
	// If a mission is selected, draw its descriptive text.
	if(availableIt != available.end())
		wrap.Wrap(availableIt->Description());
	else if(acceptedIt != accepted.end())
		wrap.Wrap(acceptedIt->Description());
	else
		return;
	wrap.Draw(Point(-190., Screen::Bottom() - 213.), *GameData::Colors().Get("bright"));
}



bool MissionPanel::CanAccept() const
{
	if(availableIt == available.end())
		return false;
	
	return availableIt->HasSpace(player);
}



void MissionPanel::Accept()
{
	const Mission &toAccept = *availableIt;
	int cargoToSell = 0;
	if(toAccept.CargoSize())
		cargoToSell = toAccept.CargoSize() - player.Cargo().Free();
	int crewToFire = 0;
	if(toAccept.Passengers())
		crewToFire = toAccept.Passengers() - player.Cargo().Bunks();
	if(cargoToSell > 0 || crewToFire > 0)
	{
		ostringstream out;
		if(cargoToSell > 0 && crewToFire > 0)
			out << "You must fire " << crewToFire << " of your flagship's non-essential crew members and sell "
				<< cargoToSell << " tons of ordinary commodities to make room for this mission. Continue?";
		else if(crewToFire > 0)
			out << "You must fire " << crewToFire
				<< " of your flagship's non-essential crew members to make room for this mission. Continue?";
		else
			out << "You must sell " << cargoToSell
				<< " tons of ordinary commodities to make room for this mission. Continue?";
		GetUI()->Push(new Dialog(this, &MissionPanel::MakeSpaceAndAccept, out.str()));
		return;
	}
	
	++availableIt;
	player.AcceptJob(toAccept);
	if(availableIt == available.end() && !available.empty())
		--availableIt;
}



void MissionPanel::MakeSpaceAndAccept()
{
	const Mission &toAccept = *availableIt;
	int cargoToSell = toAccept.CargoSize() - player.Cargo().Free();
	int crewToFire = toAccept.Passengers() - player.Cargo().Bunks();
	
	if(crewToFire > 0)
		player.Flagship()->AddCrew(-crewToFire);
	
	for(const auto &it : player.Cargo().Commodities())
	{
		if(cargoToSell <= 0)
			break;
		
		int toSell = min(cargoToSell, it.second);
		int64_t price = player.GetSystem()->Trade(it.first);
		
		int64_t basis = player.GetBasis(it.first, toSell);
		player.AdjustBasis(it.first, -basis);
		player.Cargo().Transfer(it.first, toSell);
		player.Accounts().AddCredits(toSell * price);
	}
	
	player.UpdateCargoCapacities();
	Accept();
}



void MissionPanel::AbortMission()
{
	if(acceptedIt != accepted.end())
	{
		const Mission &toAbort = *acceptedIt;
		++acceptedIt;
		player.RemoveMission(Mission::FAIL, toAbort, GetUI());
		if(acceptedIt == accepted.end() && !accepted.empty())
			--acceptedIt;
	}
}


	
int MissionPanel::AcceptedVisible() const
{
	int count = 0;
	for(const Mission &mission : accepted)
		count += mission.IsVisible();
	return count;
}
