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
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "LineShader.h"
#include "Mission.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Preferences.h"
#include "RingShader.h"
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
	const int SIDE_WIDTH = 280;
}



MissionPanel::MissionPanel(PlayerInfo &player)
	: MapPanel(player),
	available(player.AvailableJobs()),
	accepted(player.Missions()),
	availableIt(player.AvailableJobs().begin()),
	acceptedIt(player.AvailableJobs().empty() ? accepted.begin() : accepted.end())
{
	while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
		++acceptedIt;
	
	wrap.SetWrapWidth(380);
	wrap.SetFont(FontSet::Get(14));
	wrap.SetAlignment(WrappedText::JUSTIFIED);

	// Select the first available or accepted mission in the currently selected
	// system, or along the travel plan.
	if(!FindMissionForSystem(selectedSystem) && player.HasTravelPlan())
	{
		const auto &tp = player.TravelPlan();
		for(auto it = tp.crbegin(); it != tp.crend(); ++it)
			if(FindMissionForSystem(*it))
				break;
	}

	// Auto select the destination system for the current mission.
	if(availableIt != available.end())
		selectedSystem = availableIt->Destination()->GetSystem();
	else if(acceptedIt != accepted.end())
		selectedSystem = acceptedIt->Destination()->GetSystem();

	// Center the system slightly above the center of the screen because the
	// lower panel is taking up more space than the upper one.
	center = Point(0., -80.) - selectedSystem->Position();
}



MissionPanel::MissionPanel(const MapPanel &panel)
	: MapPanel(panel),
	available(player.AvailableJobs()),
	accepted(player.Missions()),
	availableIt(player.AvailableJobs().begin()),
	acceptedIt(player.AvailableJobs().empty() ? accepted.begin() : accepted.end()),
	availableScroll(0), acceptedScroll(0), dragSide(0)
{
	// In this view, always color systems based on player reputation.
	commodity = SHOW_REPUTATION;
	
	while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
		++acceptedIt;
	
	wrap.SetWrapWidth(380);
	wrap.SetFont(FontSet::Get(14));
	wrap.SetAlignment(WrappedText::JUSTIFIED);

	// Select the first available or accepted mission in the currently selected
	// system, or along the travel plan.
	if(!FindMissionForSystem(selectedSystem)
		&& !(player.GetSystem() != selectedSystem && FindMissionForSystem(player.GetSystem()))
		&& player.HasTravelPlan())
	{
		const auto &tp = player.TravelPlan();
		for(auto it = tp.crbegin(); it != tp.crend(); ++it)
			if(FindMissionForSystem(*it))
				break;
	}
}



void MissionPanel::Step()
{
	DoHelp("jobs");
}



void MissionPanel::Draw()
{
	MapPanel::Draw();
	
	Color routeColor(.2, .1, 0., 0.);
	const System *system = selectedSystem;
	while(distance.Days(system) > 0)
	{
		const System *next = distance.Route(system);
		
		Point from = Zoom() * (next->Position() + center);
		Point to = Zoom() * (system->Position() + center);
		Point unit = (from - to).Unit() * 7.;
		from -= unit;
		to += unit;
		
		LineShader::Draw(from, to, 5., routeColor);
		
		system = next;
	}
	
	DrawKey();
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
	
	const Set<Color> &colors = GameData::Colors();
	const Color &availableColor = *colors.Get("available back");
	const Color &unavailableColor = *colors.Get("unavailable back");
	const Color &currentColor = *colors.Get("active back");
	const Color &blockedColor = *colors.Get("blocked back");
	if(availableIt != available.end() && availableIt->Destination())
		DrawMissionSystem(*availableIt, CanAccept() ? availableColor : unavailableColor);
	if(acceptedIt != accepted.end() && acceptedIt->Destination())
		DrawMissionSystem(*acceptedIt, IsSatisfied(*acceptedIt) ? currentColor : blockedColor);
	
	DrawButtons("is missions");
}



// Only override the ones you need; the default action is to return false.
bool MissionPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == 'a' && CanAccept())
	{
		Accept();
		return true;
	}
	else if(key == 'A' || (key == 'a' && (mod & KMOD_SHIFT)))
	{
		if(acceptedIt != accepted.end() && acceptedIt->IsVisible())
			GetUI()->Push(new Dialog(this, &MissionPanel::AbortMission,
				"Abort mission \"" + acceptedIt->Name() + "\"?"));
		return true;
	}
	else if(key == SDLK_LEFT && availableIt == available.end())
	{
		acceptedIt = accepted.end();
		availableIt = available.begin();
	}
	else if(key == SDLK_RIGHT && acceptedIt == accepted.end() && AcceptedVisible())
	{
		availableIt = available.end();
		acceptedIt = accepted.begin();
		while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
			++acceptedIt;
	}
	else if(key == SDLK_UP)
	{
		SelectAnyMission();
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
	else if(key == SDLK_DOWN && !SelectAnyMission())
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
	else
		return MapPanel::KeyDown(key, mod, command);
	
	if(availableIt != available.end())
		selectedSystem = availableIt->Destination()->GetSystem();
	else if(acceptedIt != accepted.end())
		selectedSystem = acceptedIt->Destination()->GetSystem();
	if(selectedSystem)
		center = Point(0., -80.) - selectedSystem->Position();
	
	return true;
}



bool MissionPanel::Click(int x, int y, int clicks)
{
	dragSide = 0;
	
	if(x > Screen::Right() - 80 && y > Screen::Bottom() - 50)
		return DoKey('p');
	
	if(x < Screen::Left() + SIDE_WIDTH)
	{
		unsigned index = max(0, (y + static_cast<int>(availableScroll) - 36 - Screen::Top()) / 20);
		if(index < available.size())
		{
			availableIt = available.begin();
			while(index--)
				++availableIt;
			acceptedIt = accepted.end();
			dragSide = -1;
			selectedSystem = availableIt->Destination()->GetSystem();
			center = Point(0., -80.) - selectedSystem->Position();
			return true;
		}
	}
	else if(x >= Screen::Right() - SIDE_WIDTH)
	{
		int index = max(0, (y + static_cast<int>(acceptedScroll) - 36 - Screen::Top()) / 20);
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
			selectedSystem = acceptedIt->Destination()->GetSystem();
			center = Point(0., -80.) - selectedSystem->Position();
			return true;
		}
	}
	
	// Figure out if a system was clicked on.
	Point click = Point(x, y) / Zoom() - center;
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
		// If you just aborted your last mission, it is possible that neither
		// iterator is valid. In that case, start over from the beginning.
		if(availableIt == available.end() && acceptedIt == accepted.end())
		{
			if(!available.empty())
				availableIt = available.begin();
			else
				acceptedIt = accepted.begin();
		}
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
			
			if(availableIt != available.end() && availableIt->Destination()->IsInSystem(system))
				break;
			if(acceptedIt != accepted.end() && acceptedIt->Destination()->IsInSystem(system))
				break;
		}
		// Make sure invisible missions are never selected, even if there were
		// no other missions in this system.
		if(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
			acceptedIt = accepted.end();
	}
	
	return true;
}



bool MissionPanel::Drag(double dx, double dy)
{
	if(dragSide < 0)
	{
		availableScroll = max(0.,
			min(available.size() * 20. + 190. - Screen::Height(),
				availableScroll - dy));
	}
	else if(dragSide > 0)
	{
		acceptedScroll = max(0.,
			min(accepted.size() * 20. + 160. - Screen::Height(),
				acceptedScroll - dy));
	}
	else
		MapPanel::Drag(dx, dy);
	
	return true;
}



bool MissionPanel::Hover(int x, int y)
{
	dragSide = 0;
	unsigned index = max(0, (y + static_cast<int>(availableScroll) - 36 - Screen::Top()) / 20);
	if(x < Screen::Left() + SIDE_WIDTH)
	{
		if(index < available.size())
			dragSide = -1;
	}
	else if(x >= Screen::Right() - SIDE_WIDTH)
	{
		if(static_cast<int>(index) < AcceptedVisible())
			dragSide = 1;
	}
	return true;
}



bool MissionPanel::Scroll(double dx, double dy)
{
	if(dragSide)
		return Drag(0., dy * Preferences::ScrollSpeed());
	
	return MapPanel::Scroll(dx, dy);
}



void MissionPanel::DrawKey() const
{
	const Sprite *back = SpriteSet::Get("ui/mission key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));
	
	Color bright(.6, .6);
	Color dim(.3, .3);
	const Font &font = FontSet::Get(14);
	Point angle = Point(1., 1.).Unit();
	
	Point pos(Screen::Left() + 10., Screen::Bottom() - 5. * 20. + 5.);
	Point pointerOff(5., 5.);
	Point textOff(8., -.5 * font.Height());

	const Set<Color> &colors = GameData::Colors();
	const Color COLOR[5] = {
		*colors.Get("available job"),
		*colors.Get("unavailable job"),
		*colors.Get("active mission"),
		*colors.Get("blocked mission"),
		*colors.Get("waypoint")
	};
	static const string LABEL[5] = {
		"Available job; can accept",
		"Too little space to accept",
		"Active job; go here to complete",
		"Has unfinished requirements",
		"Waypoint you must visit"
	};
	int selected = -1;
	if(availableIt != available.end())
		selected = 0 + !CanAccept();
	if(acceptedIt != accepted.end() && acceptedIt->Destination())
		selected = 2 + !IsSatisfied(*acceptedIt);
	
	for(int i = 0; i < 5; ++i)
	{
		PointerShader::Draw(pos + pointerOff, angle, 10., 18., 0., COLOR[i]);
		font.Draw(LABEL[i], pos + textOff, i == selected ? bright : dim);
		pos.Y() += 20.;
	}
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
	
	int jumps = 0;
	const vector<const System *> &plan = player.TravelPlan();
	auto it = find(plan.begin(), plan.end(), selectedSystem);
	if(it != plan.end())
		jumps = plan.end() - it;
	else if(distance.HasRoute(selectedSystem))
		jumps = distance.Days(selectedSystem);
	
	if(jumps == 1)
		text += " (1 jump away)";
	else if(jumps > 0)
		text += " (" + to_string(jumps) + " jumps away)";
	
	const Font &font = FontSet::Get(14);
	Point pos(-.5 * font.Width(text), Screen::Top() + .5 * (30. - font.Height()));
	font.Draw(text, pos, *GameData::Colors().Get("bright"));
}



void MissionPanel::DrawMissionSystem(const Mission &mission, const Color &color) const
{
	const Color &waypointColor = *GameData::Colors().Get("waypoint back");
	
	Point pos = Zoom() * (mission.Destination()->GetSystem()->Position() + center);
	RingShader::Draw(pos, 22., 20.5, color);
	for(const System *system : mission.Waypoints())
		RingShader::Draw(Zoom() * (system->Position() + center), 22., 20.5, waypointColor);
	for(const Planet *planet : mission.Stopovers())
		RingShader::Draw(Zoom() * (planet->GetSystem()->Position() + center), 22., 20.5, waypointColor);
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
		
		bool canAccept = (&list == &available ? it->HasSpace(player) : IsSatisfied(*it));
		font.Draw(it->Name(), pos,
			(!canAccept ? dim : isSelected ? selected : unselected));
	}
	
	return pos;
}



void MissionPanel::DrawMissionInfo()
{
	Information info;
	
	// The "accept / abort" button text and activation depends on what mission,
	// if any, is selected, and whether missions are available.
	if(CanAccept())
		info.SetCondition("can accept");
	else if(acceptedIt != accepted.end())
		info.SetCondition("can abort");
	
	info.SetString("cargo free", to_string(player.Cargo().Free()) + " tons");
	info.SetString("bunks free", to_string(player.Cargo().Bunks()) + " bunks");
	
	info.SetString("today", player.GetDate().ToString());
	
	const Interface *interface = GameData::Interfaces().Get("mission");
	interface->Draw(info, this);
	
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
	player.AcceptJob(toAccept, GetUI());
	if(availableIt == available.end() && !available.empty())
		--availableIt;
	
	// Check if any other jobs are available with the same destination. Prefer
	// jobs that also have the same destination planet.
	if(toAccept.Destination())
	{
		const Planet *planet = toAccept.Destination();
		const System *system = planet->GetSystem();
		for(auto it = available.begin(); it != available.end(); ++it)
			if(it->Destination() && it->Destination()->IsInSystem(system))
			{
				availableIt = it;
				if(it->Destination() == planet)
					break;
			}
	}
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
		player.Cargo().Remove(it.first, toSell);
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
		if(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
			acceptedIt = accepted.end();
	}
}


	
int MissionPanel::AcceptedVisible() const
{
	int count = 0;
	for(const Mission &mission : accepted)
		count += mission.IsVisible();
	return count;
}



bool MissionPanel::FindMissionForSystem(const System *system)
{
	if(!system)
		return false;

	acceptedIt = accepted.end();

	for(availableIt = available.begin(); availableIt != available.end(); ++availableIt)
		if(availableIt->Destination()->IsInSystem(system))
			return true;
	for(acceptedIt = accepted.begin(); acceptedIt != accepted.end(); ++acceptedIt)
		if(acceptedIt->IsVisible() && acceptedIt->Destination()->IsInSystem(system))
			return true;

	return false;
}



bool MissionPanel::SelectAnyMission()
{
	if(availableIt == available.end() && acceptedIt == accepted.end())
	{
		// no previous selection, reset
		if(!available.empty())
			availableIt = available.begin();
		else
		{
			acceptedIt = accepted.begin();
			while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
				++acceptedIt;
		}
		return availableIt != available.end() || acceptedIt != accepted.end();
	}
	return false;
}
