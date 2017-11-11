/* MapPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapPanel.h"

#include "Angle.h"
#include "Dialog.h"
#include "FogShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Galaxy.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "LineShader.h"
#include "MapDetailPanel.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "Mission.h"
#include "MissionPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Preferences.h"
#include "RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "Trade.h"
#include "UI.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>

using namespace std;

const double MapPanel::OUTER = 6.;
const double MapPanel::INNER = 3.5;



MapPanel::MapPanel(PlayerInfo &player, int commodity, const System *special)
	: player(player), distance(player),
	playerSystem(player.GetSystem()),
	selectedSystem(special ? special : player.GetSystem()),
	specialSystem(special),
	commodity(commodity)
{
	SetIsFullScreen(true);
	SetInterruptible(false);
	// Recalculate the fog each time the map is opened, just in case the player
	// bought a map since the last time they viewed the map.
	FogShader::Redraw();
	
	if(selectedSystem)
		center = Point(0., 0.) - selectedSystem->Position();
}



void MapPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	for(const auto &it : GameData::Galaxies())
		SpriteShader::Draw(it.second.GetSprite(), Zoom() * (center + it.second.Position()), Zoom());
	
	if(Preferences::Has("Hide unexplored map regions"))
		FogShader::Draw(center, Zoom(), player);
	
	DrawTravelPlan();
	
	// Draw the "visible range" circle around your current location.
	Color dimColor(.1, 0.);
	RingShader::Draw(Zoom() * (playerSystem ? playerSystem->Position() + center : center),
		(System::NEIGHBOR_DISTANCE + .5) * Zoom(), (System::NEIGHBOR_DISTANCE - .5) * Zoom(), dimColor);
	Color brightColor(.4, 0.);
	RingShader::Draw(Zoom() * (selectedSystem ? selectedSystem->Position() + center : center),
		11., 9., brightColor);
	
	DrawWormholes();
	DrawLinks();
	DrawSystems();
	DrawNames();
	DrawMissions();
	
	if(!distance.HasRoute(selectedSystem))
	{
		static const string UNAVAILABLE = "You have no available route to this system.";
		static const string UNKNOWN = "You have not yet mapped a route to this system.";
		const Font &font = FontSet::Get(18);
		Color black(0., 1.);
		Color red(1., 0., 0., 1.);
		
		const string &message = player.HasVisited(selectedSystem) ? UNAVAILABLE : UNKNOWN;
		Point point(-font.Width(message) / 2, Screen::Top() + 40);
		font.Draw(message, point + Point(1, 1), black);
		font.Draw(message, point, red);
	}
}



void MapPanel::DrawButtons(const string &condition)
{
	// Remember which buttons we're showing.
	buttonCondition = condition;
	
	// Draw the buttons to switch to other map modes.
	Information info;
	info.SetCondition(condition);
	if(player.MapZoom() == 2)
		info.SetCondition("max zoom");
	if(player.MapZoom() == -2)
		info.SetCondition("min zoom");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info, this);
}



void MapPanel::DrawMiniMap(const PlayerInfo &player, double alpha, const System *const jump[2], int step)
{
	const Font &font = FontSet::Get(14);
	Color lineColor(alpha, 0.);
	Point center = .5 * (jump[0]->Position() + jump[1]->Position());
	Point drawPos(0., Screen::Top() + 100.);
	set<const System *> seen;
	bool isLink = false;

	const Set<Color> &colors = GameData::Colors();
	Color currentColor = colors.Get("active mission")->Additive(alpha * 2.);
	Color blockedColor = colors.Get("blocked mission")->Additive(alpha * 2.);
	Color waypointColor = colors.Get("waypoint")->Additive(alpha * 2.);
	
	for(int i = 0; i < 2; ++i)
	{
		const System *system = jump[i];
		const Government *gov = system->GetGovernment();
		bool isKnown = player.KnowsName(system);
		Point from = system->Position() - center + drawPos;
		string name = isKnown ? system->Name() : "Unexplored System";
		font.Draw(name, from + Point(6., -.5 * font.Height()), lineColor);
		
		Color color = Color(.5 * alpha, 0.);
		if(player.HasVisited(system) && system->IsInhabited(player.Flagship()) && gov)
			color = Color(
				alpha * gov->GetColor().Get()[0],
				alpha * gov->GetColor().Get()[1],
				alpha * gov->GetColor().Get()[2], 0.);
		RingShader::Draw(from, 6., 3.5, color);
		
		for(const System *link : system->Links())
		{
			if(!player.HasVisited(system) && !player.HasVisited(link))
				continue;
			
			Point to = link->Position() - center + drawPos;
			Point unit = (from - to).Unit() * 7.;
			LineShader::Draw(from - unit, to + unit, 1.2, lineColor);
			
			isLink |= (link == jump[!i]);
			if(seen.count(link) || link == jump[!i])
				continue;
			seen.insert(link);
			
			gov = link->GetGovernment();
			Color color = Color(.5 * alpha, 0.);
			if(player.HasVisited(link) && link->IsInhabited(player.Flagship()) && gov)
				color = Color(
					alpha * gov->GetColor().Get()[0],
					alpha * gov->GetColor().Get()[1],
					alpha * gov->GetColor().Get()[2], 0.);
			RingShader::Draw(to, 6., 3.5, color);
		}
		
		Angle angle;
		for(const Mission &mission : player.Missions())
		{
			if(!mission.IsVisible())
				continue;
			
			if(mission.Destination()->IsInSystem(system))
			{
				bool blink = false;
				if(mission.Deadline())
				{
					int days = min(5, mission.Deadline() - player.GetDate()) + 1;
					if(days > 0)
						blink = (step % (10 * days) > 5 * days);
				}
				if(!blink)
				{
					bool isSatisfied = IsSatisfied(player, mission);
					DrawPointer(from, angle, isSatisfied ? currentColor : blockedColor, false);
				}
			}
			
			for(const System *waypoint : mission.Waypoints())
				if(waypoint == system)
					DrawPointer(from, angle, waypointColor, false);
			for(const Planet *stopover : mission.Stopovers())
				if(stopover->IsInSystem(system))
					DrawPointer(from, angle, waypointColor, false);
		}
	}
	
	Point from = jump[0]->Position() - center + drawPos;
	Point to = jump[1]->Position() - center + drawPos;
	Point unit = (to - from).Unit();
	from += 7. * unit;
	to -= 7. * unit;
	Color bright(2. * alpha, 0.);
	if(!isLink)
	{
		double length = (to - from).Length();
		int segments = static_cast<int>(length / 15.);
		for(int i = 0; i < segments; ++i)
			LineShader::Draw(
				from + unit * ((i * length) / segments + 2.),
				from + unit * (((i + 1) * length) / segments - 2.),
				1.2, bright);
	}
	LineShader::Draw(to, to + Angle(-30.).Rotate(unit) * -10., 1.2, bright);
	LineShader::Draw(to, to + Angle(30.).Rotate(unit) * -10., 1.2, bright);
}



bool MapPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == 's' && buttonCondition != "is shipyards")
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapShipyardPanel(*this));
	}
	else if(key == 'o' && buttonCondition != "is outfitters")
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapOutfitterPanel(*this));
	}
	else if(key == 'i' && buttonCondition != "is missions")
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MissionPanel(*this));
	}
	else if(key == 'p' && buttonCondition != "is ports")
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapDetailPanel(*this));
	}
	else if(key == 'f')
	{
		GetUI()->Push(new Dialog(
			this, &MapPanel::Find, "Search for:"));
		return true;
	}
	else if(key == '+' || key == '=')
		player.SetMapZoom(min(2, player.MapZoom() + 1));
	else if(key == '-')
		player.SetMapZoom(max(-2, player.MapZoom() - 1));
	else
		return false;
	
	return true;
}



bool MapPanel::Click(int x, int y, int clicks)
{
	// Figure out if a system was clicked on.
	Point click = Point(x, y) / Zoom() - center;
	for(const auto &it : GameData::Systems())
		if(click.Distance(it.second.Position()) < 10.
				&& (player.HasSeen(&it.second) || &it.second == specialSystem))
		{
			Select(&it.second);
			break;
		}
	
	return true;
}



bool MapPanel::Drag(double dx, double dy)
{
	center += Point(dx, dy) / Zoom();
	return true;
}



bool MapPanel::Scroll(double dx, double dy)
{
	// The mouse should be pointing to the same map position before and after zooming.
	Point mouse = UI::GetMouse();
	Point anchor = mouse / Zoom() - center;
	if(dy > 0.)
		player.SetMapZoom(min(2, player.MapZoom() + 1));
	else
		player.SetMapZoom(max(-2, player.MapZoom() - 1));
	
	// Now, Zoom() has changed (unless at one of the limits). But, we still want
	// anchor to be the same, so:
	center = mouse / Zoom() - anchor;
	return true;
}



Color MapPanel::MapColor(double value)
{
	if(std::isnan(value))
		return UninhabitedColor();
	
	value = min(1., max(-1., value));
	if(value < 0.)
		return Color(
			.12 + .12 * value,
			.48 + .36 * value,
			.48 - .12 * value,
			.4);
	else
		return Color(
			.12 + .48 * value,
			.48,
			.48 - .48 * value,
			.4);
}



Color MapPanel::ReputationColor(double reputation, bool canLand, bool hasDominated)
{
	// If the system allows you to land, always show it in blue even if the
	// government is hostile.
	if(canLand)
		reputation = max(reputation, 0.);
	
	if(hasDominated)
		return Color(.1, .6, 0., .4);
	else if(reputation < 0.)
	{
		reputation = min(1., .1 * log(1. - reputation) + .1);
		return Color(.6, .4 * (1. - reputation), 0., .4);
	}
	else if(!canLand)
		return Color(.6, .54, 0., .4);
	else
	{
		reputation = min(1., .1 * log(1. + reputation) + .1);
		return Color(0., .6 * (1. - reputation), .6, .4);
	}
}



Color MapPanel::GovernmentColor(const Government *government)
{
	if(!government)
		return UninhabitedColor();
	
	return Color(
		.6 * government->GetColor().Get()[0],
		.6 * government->GetColor().Get()[1],
		.6 * government->GetColor().Get()[2],
		.4);
}



Color MapPanel::UninhabitedColor()
{
	return Color(.2, 0.);
}



Color MapPanel::UnexploredColor()
{
	return Color(.1, 0.);
}



double MapPanel::SystemValue(const System *system) const
{
	return 0.;
}



void MapPanel::Select(const System *system)
{
	if(!system)
		return;
	selectedSystem = system;
	vector<const System *> &plan = player.TravelPlan();
	if(!player.Flagship() || (!plan.empty() && system == plan.front()))
		return;
	
	bool isJumping = player.Flagship()->IsEnteringHyperspace();
	const System *source = isJumping ? player.Flagship()->GetTargetSystem() : player.GetSystem();
	
	bool shift = (SDL_GetModState() & KMOD_SHIFT) && !plan.empty();
	if(system == source && !shift)
	{
		plan.clear();
		if(!isJumping)
			player.Flagship()->SetTargetSystem(nullptr);
		else
			plan.push_back(source);
	}
	else if(shift)
	{
		DistanceMap localDistance(player, plan.front());
		if(localDistance.Days(system) <= 0)
			return;
		
		auto it = plan.begin();
		while(system != *it)
		{
			it = ++plan.insert(it, system);
			system = localDistance.Route(system);
		}
	}
	else if(distance.Days(system) > 0)
	{
		plan.clear();
		if(!isJumping)
			player.Flagship()->SetTargetSystem(nullptr);
		
		while(system != source)
		{
			plan.push_back(system);
			system = distance.Route(system);
		}
		if(isJumping)
			plan.push_back(source);
	}
}



void MapPanel::Find(const string &name)
{
	int bestIndex = 9999;
	for(const auto &it : GameData::Systems())
		if(player.HasVisited(&it.second))
		{
			int index = Search(it.first, name);
			if(index >= 0 && index < bestIndex)
			{
				bestIndex = index;
				selectedSystem = &it.second;
				center = Zoom() * (Point() - selectedSystem->Position());
				if(!index)
				{
					selectedPlanet = nullptr;
					return;
				}
			}
		}
	for(const auto &it : GameData::Planets())
		if(player.HasVisited(it.second.GetSystem()))
		{
			int index = Search(it.first, name);
			if(index >= 0 && index < bestIndex)
			{
				bestIndex = index;
				selectedSystem = it.second.GetSystem();
				center = Zoom() * (Point() - selectedSystem->Position());
				if(!index)
				{
					selectedPlanet = &it.second;
					return;
				}
			}
		}
}



double MapPanel::Zoom() const
{
	return pow(1.5, player.MapZoom());
}



// Check whether the NPC and waypoint conditions of the given mission have
// been satisfied.
bool MapPanel::IsSatisfied(const Mission &mission) const
{
	return IsSatisfied(player, mission);
}



bool MapPanel::IsSatisfied(const PlayerInfo &player, const Mission &mission)
{
	return mission.IsSatisfied(player) && !mission.HasFailed(player);
}



int MapPanel::Search(const string &str, const string &sub)
{
	auto it = search(str.begin(), str.end(), sub.begin(), sub.end(),
		[](char a, char b) { return toupper(a) == toupper(b); });
	return (it == str.end() ? -1 : it - str.begin());
}



void MapPanel::DrawTravelPlan()
{
	if(!playerSystem)
		return;
	
	Color defaultColor(.5, .4, 0., 0.);
	Color outOfFlagshipFuelRangeColor(.55, .1, .0, 0.);
	Color withinFleetFuelRangeColor(.2, .5, .0, 0.);
	Color wormholeColor(0.5, 0.2, 0.9, 1.);
	
	// At each point in the path, we'll keep track of how many ships in the
	// fleet are able to make it this far.
	Ship *flagship = player.Flagship();
	if(!flagship)
		return;
	bool stranded = false;
	bool hasEscort = false;
	map<const Ship *, double> fuel;
	for(const shared_ptr<Ship> &it : player.Ships())
		if(!it->IsParked() && !it->CanBeCarried() && it->GetSystem() == flagship->GetSystem())
		{
			if(it->IsDisabled())
			{
				stranded = true;
				continue;
			}
			
			fuel[it.get()] = it->Fuel() * it->Attributes().Get("fuel capacity");
			hasEscort |= (it.get() != flagship);
		}
	stranded |= !hasEscort;
	
	const System *previous = playerSystem;
	for(int i = player.TravelPlan().size() - 1; i >= 0; --i)
	{
		const System *next = player.TravelPlan()[i];
		bool isHyper = previous->Links().count(next);
		bool isJump = !isHyper && previous->Neighbors().count(next);
		bool isWormhole = false;
		for(const StellarObject &object : previous->Objects())
			isWormhole |= (object.GetPlanet() && player.HasVisited(object.GetPlanet())
				&& !object.GetPlanet()->Description().empty()
				&& player.HasVisited(previous) && player.HasVisited(next)
				&& object.GetPlanet()->WormholeDestination(previous) == next);
		
		if(!isHyper && !isJump && !isWormhole)
			break;
		
		// Wormholes cost nothing to grow through. If this is not a wormhole,
		// check how much fuel every ship will expend to go through it.
		if(!isWormhole)
			for(auto &it : fuel)
				if(it.second >= 0.)
				{
					double cost = isJump ? it.first->JumpDriveFuel() : it.first->HyperdriveFuel();
					if(!cost || cost > it.second)
					{
						it.second = -1.;
						stranded = true;
					}
					else
						it.second -= cost;
				}
		
		// Color the path green if all ships can make it. Color it yellow if
		// the flagship can make it, and red if the flagship cannot.
		Color drawColor = outOfFlagshipFuelRangeColor;
		if(isWormhole)
			drawColor = wormholeColor;
		else if(!stranded)
			drawColor = withinFleetFuelRangeColor;
		else if(fuel[flagship] >= 0.)
			drawColor = defaultColor;
		
		Point from = Zoom() * (next->Position() + center);
		Point to = Zoom() * (previous->Position() + center);
		Point unit = (from - to).Unit() * 7.;
		LineShader::Draw(from - unit, to + unit, 3., drawColor);
		
		previous = next;
	}
}



void MapPanel::DrawWormholes()
{
	Color wormholeColor(0.5, 0.2, 0.9, 1.);
	Color wormholeDimColor(0.5 / 3., 0.2 / 3., 0.9 / 3., 1.);
	const double wormholeWidth = 1.2;
	const double wormholeLength = 4.;
	const double wormholeArrowHeadRatio = .3;
	
	map<const System *, const System *> drawn;
	for(const auto &it : GameData::Systems())
	{
		const System *previous = &it.second;
		for(const StellarObject &object : previous->Objects())
			if(object.GetPlanet() && object.GetPlanet()->IsWormhole() && player.HasVisited(object.GetPlanet()))
			{
				// Wormholes with no description should not be drawn.
				if(object.GetPlanet()->Description().empty())
					continue;
				
				const System *next = object.GetPlanet()->WormholeDestination(previous);
				// Only draw a wormhole if both systems have been visited.
				if(!player.HasVisited(previous) || !player.HasVisited(next))
					continue;
				
				drawn[previous] = next;
				Point from = Zoom() * (previous->Position() + center);
				Point to = Zoom() * (next->Position() + center);
				Point unit = (from - to).Unit() * 7.;
				from -= unit;
				to += unit;
				
				Angle left(45.);
				Angle right(-45.);
				
				Point wormholeUnit = Zoom() * wormholeLength * unit;
				Point arrowLeft = left.Rotate(wormholeUnit * wormholeArrowHeadRatio);
				Point arrowRight = right.Rotate(wormholeUnit * wormholeArrowHeadRatio);
				
				// Don't double-draw the links.
				if(drawn[next] != previous)
					LineShader::Draw(from, to, wormholeWidth, wormholeDimColor);
				LineShader::Draw(from - wormholeUnit + arrowLeft, from - wormholeUnit, wormholeWidth, wormholeColor);
				LineShader::Draw(from - wormholeUnit + arrowRight, from - wormholeUnit, wormholeWidth, wormholeColor);
				LineShader::Draw(from, from - (wormholeUnit + Zoom() * 0.1 * unit), wormholeWidth, wormholeColor);
			}
	}
}



void MapPanel::DrawLinks()
{
	// Draw the links between the systems.
	Color closeColor(.6, .6);
	Color farColor(.3, .3);
	for(const auto &it : GameData::Systems())
	{
		const System *system = &it.second;
		if(!player.HasSeen(system))
			continue;
		
		for(const System *link : system->Links())
			if(link < system || !player.HasSeen(link))
			{
				// Only draw links between two systems if one of the two is
				// visited. Also, avoid drawing twice by only drawing in the
				// direction of increasing pointer values.
				if(!player.HasVisited(system) && !player.HasVisited(link))
					continue;
				
				Point from = Zoom() * (system->Position() + center);
				Point to = Zoom() * (link->Position() + center);
				Point unit = (from - to).Unit() * 7.;
				from -= unit;
				to += unit;
				
				bool isClose = (system == playerSystem || link == playerSystem);
				LineShader::Draw(from, to, 1.2, isClose ? closeColor : farColor);
			}
	}
}



void MapPanel::DrawSystems()
{
	if(commodity == SHOW_GOVERNMENT)
		closeGovernments.clear();
	
	// Draw the circles for the systems, colored based on the selected criterion,
	// which may be government, services, or commodity prices.
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		// Referring to a non-existent system in a mission can create a spurious
		// system record. Ignore those.
		if(system.Name().empty())
			continue;
		if(!player.HasSeen(&system) && &system != specialSystem)
			continue;
		
		Point pos = Zoom() * (system.Position() + center);
		
		Color color = UninhabitedColor();
		if(!player.HasVisited(&system))
			color = UnexploredColor();
		else if(system.IsInhabited(player.Flagship()) || commodity == SHOW_SPECIAL)
		{
			if(commodity >= SHOW_SPECIAL)
			{
				double value = 0.;
				if(commodity >= 0)
				{
					const Trade::Commodity &com = GameData::Commodities()[commodity];
					double price = system.Trade(com.name);
					if(!price)
						value = numeric_limits<double>::quiet_NaN();
					else
						value = (2. * (price - com.low)) / (com.high - com.low) - 1.;
				}
				else if(commodity == SHOW_SHIPYARD)
				{
					double size = 0;
					for(const StellarObject &object : system.Objects())
						if(object.GetPlanet())
							size += object.GetPlanet()->Shipyard().size();
					value = size ? min(10., size) / 10. : -1.;
				}
				else if(commodity == SHOW_OUTFITTER)
				{
					double size = 0;
					for(const StellarObject &object : system.Objects())
						if(object.GetPlanet())
							size += object.GetPlanet()->Outfitter().size();
					value = size ? min(60., size) / 60. : -1.;
				}
				else if(commodity == SHOW_VISITED)
				{
					bool all = true;
					bool some = false;
					for(const StellarObject &object : system.Objects())
						if(object.GetPlanet() && !object.GetPlanet()->IsWormhole())
						{
							bool visited = player.HasVisited(object.GetPlanet());
							all &= visited;
							some |= visited;
						}
					value = -1 + some + all;
				}
				else
					value = SystemValue(&system);
				
				color = MapColor(value);
			}
			else if(commodity == SHOW_GOVERNMENT)
			{
				const Government *gov = system.GetGovernment();
				color = GovernmentColor(gov);
				
				// For every government that is draw, keep track of how close it
				// is to the center of the view. The four closest governments
				// will be displayed in the key.
				double distance = pos.Length();
				auto it = closeGovernments.find(gov);
				if(it == closeGovernments.end())
					closeGovernments[gov] = distance;
				else
					it->second = min(it->second, distance);
			}
			else
			{
				double reputation = system.GetGovernment()->Reputation();
				
				// A system should show up as dominated if it contains at least
				// one inhabited planet and all inhabited planets have been
				// dominated. It should show up as restricted if you cannot land
				// on any of the planets that have spaceports.
				bool hasDominated = true;
				bool isInhabited = false;
				bool canLand = false;
				bool hasSpaceport = false;
				for(const StellarObject &object : system.Objects())
					if(object.GetPlanet())
					{
						const Planet *planet = object.GetPlanet();
						hasSpaceport |= !planet->IsWormhole() && planet->HasSpaceport();
						if(planet->IsWormhole() || !planet->IsAccessible(player.Flagship()))
							continue;
						canLand |= planet->CanLand() && planet->HasSpaceport();
						isInhabited |= planet->IsInhabited();
						hasDominated &= (!planet->IsInhabited()
							|| GameData::GetPolitics().HasDominated(planet));
					}
				hasDominated &= (isInhabited && canLand);
				// Some systems may count as "inhabited" but not contain any
				// planets with spaceports. Color those as if they're
				// uninhabited to make it clear that no fuel is available there.
				if(hasSpaceport || hasDominated)
					color = ReputationColor(reputation, canLand, hasDominated);
			}
		}
		
		RingShader::Draw(pos, OUTER, INNER, color);
	}
}



void MapPanel::DrawNames()
{
	// Don't draw if too small.
	if(Zoom() <= 0.5)
		return;
	
	// Draw names for all systems you have visited.
	const Font &font = FontSet::Get((Zoom() > 2.0) ? 18 : 14);
	Color closeColor(.6, .6);
	Color farColor(.3, .3);
	Point offset((Zoom() > 2.0) ? 8. : 6., -.5 * font.Height());
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		if(!player.KnowsName(&system) || system.Name().empty())
			continue;
		
		font.Draw(system.Name(), Zoom() * (system.Position() + center) + offset,
			(&system == playerSystem) ? closeColor : farColor);
	}
}



void MapPanel::DrawMissions()
{
	// Draw a pointer for each active or available mission.
	map<const System *, Angle> angle;
	Color black(0., 1.);
	Color white(1., 1.);
	
	const Set<Color> &colors = GameData::Colors();
	const Color &availableColor = *colors.Get("available job");
	const Color &unavailableColor = *colors.Get("unavailable job");
	const Color &currentColor = *colors.Get("active mission");
	const Color &blockedColor = *colors.Get("blocked mission");
	const Color &waypointColor = *colors.Get("waypoint");
	for(const Mission &mission : player.AvailableJobs())
	{
		const System *system = mission.Destination()->GetSystem();
		DrawPointer(system, angle[system], mission.HasSpace(player) ? availableColor : unavailableColor);
	}
	++step;
	for(const Mission &mission : player.Missions())
	{
		if(!mission.IsVisible())
			continue;
		
		const System *system = mission.Destination()->GetSystem();
		bool blink = false;
		if(mission.Deadline())
		{
			int days = min(5, mission.Deadline() - player.GetDate()) + 1;
			if(days > 0)
				blink = (step % (10 * days) > 5 * days);
		}
		bool isSatisfied = IsSatisfied(player, mission);
		DrawPointer(system, angle[system], blink ? black : isSatisfied ? currentColor : blockedColor, isSatisfied);
		
		for(const System *waypoint : mission.Waypoints())
			DrawPointer(waypoint, angle[waypoint], waypointColor);
		for(const Planet *stopover : mission.Stopovers())
			DrawPointer(stopover->GetSystem(), angle[stopover->GetSystem()], waypointColor);
	}
	if(specialSystem)
	{
		// The special system pointer is larger than the others.
		Angle a = (angle[specialSystem] += Angle(30.));
		Point pos = Zoom() * (specialSystem->Position() + center);
		PointerShader::Draw(pos, a.Unit(), 20., 27., -4., black);
		PointerShader::Draw(pos, a.Unit(), 11.5, 21.5, -6., white);
	}
}



void MapPanel::DrawPointer(const System *system, Angle &angle, const Color &color, bool bigger)
{
	DrawPointer(Zoom() * (system->Position() + center), angle, color, true, bigger);
}



void MapPanel::DrawPointer(Point position, Angle &angle, const Color &color, bool drawBack, bool bigger)
{
	static const Color black(0., 1.);
	
	angle += Angle(30.);
	if(drawBack)
		PointerShader::Draw(position, angle.Unit(), 14. + bigger, 19. + 2 * bigger, -4., black);
	PointerShader::Draw(position, angle.Unit(), 8. + bigger, 15. + 2 * bigger, -6., color);
}
