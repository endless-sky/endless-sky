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
#include "DotShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Galaxy.h"
#include "GameData.h"
#include "Government.h"
#include "LineShader.h"
#include "Mission.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Screen.h"
#include "Ship.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "Trade.h"
#include "UI.h"

#include <algorithm>
#include <cctype>

using namespace std;

namespace {
	bool Contains(const string &str, const string &sub)
	{
		auto it = search(str.begin(), str.end(), sub.begin(), sub.end(),
			[](char a, char b) { return toupper(a) == toupper(b); });
		return (it != str.end());
	}
}

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
	
	if(selectedSystem)
		center = Point(0., 0.) - Zoom() * (selectedSystem->Position());
}



void MapPanel::SetCommodity(int index)
{
	commodity = index;
}



void MapPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	for(const auto &it : GameData::Galaxies())
		SpriteShader::Draw(it.second.GetSprite(), Zoom() * (center + it.second.Position()), Zoom());
	
	DrawTravelPlan();
	
	// Draw the "visible range" circle around your current location.
	Color dimColor(.1, 0.);
	DotShader::Draw(Zoom() * (playerSystem ? playerSystem->Position() + center : center),
		100.5 * Zoom(), 99.5 * Zoom(), dimColor);
	Color brightColor(.4, 0.);
	DotShader::Draw(Zoom() * (selectedSystem ? selectedSystem->Position() + center : center),
		11., 9., brightColor);
	
	DrawWormholes();
	DrawLinks();
	DrawSystems();
	DrawNames();
	DrawMissions();
	
	if(!distance.HasRoute(selectedSystem))
	{
		const Font &font = FontSet::Get(18);
		
		static const string NO_ROUTE = "You have not yet mapped a route to this system.";
		Color black(0., 1.);
		Color red(1., 0., 0., 1.);
		Point point(-font.Width(NO_ROUTE) / 2, Screen::Top() + 40);
		
		font.Draw(NO_ROUTE, point + Point(1, 1), black);
		font.Draw(NO_ROUTE, point, red);
	}
}



bool MapPanel::Click(int x, int y)
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



bool MapPanel::Drag(int dx, int dy)
{
	center += Point(dx, dy) / Zoom();
	return true;
}



bool MapPanel::Scroll(int dx, int dy)
{
	// The mouse should be pointing to the same map position before and after zooming.
	Point mouse = UI::GetMouse();
	Point anchor = mouse / Zoom() - center;
	if(dy > 0)
		ZoomMap();
	else
		UnzoomMap();
	// Now, Zoom() has changed (unless at one of the limits). But, we still want
	// anchor to be the same, so:
	center = mouse / Zoom() - anchor;
	return true;
}



Color MapPanel::MapColor(double value)
{
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
	return 0;
}



void MapPanel::Select(const System *system)
{
	if(!system)
		return;
	selectedSystem = system;
	
	bool shift = (SDL_GetModState() & KMOD_SHIFT) && player.HasTravelPlan();
	if(system == playerSystem && !shift)
		player.ClearTravel();
	else if((distance.Distance(system) > 0 || shift) && player.Flagship())
	{
		if(shift)
		{
			vector<const System *> oldPath = player.TravelPlan();
			DistanceMap localDistance(player, oldPath.front());
			player.ClearTravel();
			
			while(system != oldPath.front())
			{
				player.AddTravel(system);
				system = localDistance.Route(system);
			}
			for(const System *it : oldPath)
				player.AddTravel(it);
		}
		else if(playerSystem)
		{
			player.ClearTravel();
			while(system != playerSystem)
			{
				player.AddTravel(system);
				system = distance.Route(system);
			}
		}
	}
}



const Planet *MapPanel::Find(const string &name)
{
	for(const auto &it : GameData::Systems())
		if(player.HasVisited(&it.second) && Contains(it.first, name))
		{
			selectedSystem = &it.second;
			center = Zoom() * (Point() - selectedSystem->Position());
			return nullptr;
		}
	for(const auto &it : GameData::Planets())
		if(player.HasVisited(it.second.GetSystem()) && Contains(it.first, name))
		{
			selectedSystem = it.second.GetSystem();
			center = Zoom() * (Point() - selectedSystem->Position());
			return &it.second;
		}
	return nullptr;
}



void MapPanel::ZoomMap()
{
	if(zoom < maxZoom)
		zoom++;
}



void MapPanel::UnzoomMap()
{
	if(zoom > -maxZoom)
		zoom--;
}



double MapPanel::Zoom() const
{
	return pow(1.5, zoom);
}



bool MapPanel::ZoomIsMax() const
{
	return (zoom == maxZoom);
}



bool MapPanel::ZoomIsMin() const
{
	return (zoom == -maxZoom);
}



// Check whether the NPC and waypoint conditions of the given mission have
// been satisfied.
bool MapPanel::IsSatisfied(const Mission &mission) const
{
	for(const NPC &npc : mission.NPCs())
		if(!npc.HasSucceeded(player.GetSystem()))
			return false;
	
	return mission.Waypoints().empty() && mission.Stopovers().empty();
}



void MapPanel::DrawTravelPlan() const
{
	Color defaultColor(.5, .4, 0., 0.);
	Color outOfFlagshipFuelRangeColor(.55, .1, .0, 0.);
	Color withinFleetFuelRangeColor(.2, .5, .0, 0.);
	Color wormholeColor(0.5, 0.2, 0.9, 1.);
	
	Ship *ship = player.Flagship();
	bool hasHyper = ship ? ship->Attributes().Get("hyperdrive") : false;
	bool hasJump = ship ? ship->Attributes().Get("jump drive") : false;
	
	// Find out how much fuel your ship and your escorts use per jump.
	double flagshipCapacity = 0.;
	if(ship)
		flagshipCapacity = ship->Attributes().Get("fuel capacity") * ship->Fuel();
	double flagshipJumpFuel = 0.;
	if(ship)
		flagshipJumpFuel = hasHyper ? ship->Attributes().Get("scram drive") ? 150. : 100. : 200.;
	double escortCapacity = 0.;
	double escortJumpFuel = 1.;
	bool escortHasJump = false;
	// Skip your flagship, parked ships, and fighters.
	for(const shared_ptr<Ship> &it : player.Ships())
		if(it.get() != ship && !it->IsParked() && !it->CanBeCarried())
		{
			double capacity = it->Attributes().Get("fuel capacity") * it->Fuel();
			double jumpFuel = it->Attributes().Get("hyperdrive") ?
				it->Attributes().Get("scram drive") ? 150. : 100. : 200.;
			if(escortJumpFuel < 100. || capacity / jumpFuel < escortCapacity / escortJumpFuel)
			{
				escortCapacity = capacity;
				escortJumpFuel = jumpFuel;
				escortHasJump = it->Attributes().Get("jump drive");
			}
		}
	
	// Draw your current travel plan.
	if(!playerSystem)
		return;
	const System *previous = playerSystem;
	for(int i = player.TravelPlan().size() - 1; i >= 0; --i)
	{
		const System *next = player.TravelPlan()[i];
		
		// Figure out what kind of jump this is, and check if the player is able
		// to make jumps of that kind.
		bool isHyper = 
			(find(previous->Links().begin(), previous->Links().end(), next)
				!= previous->Links().end());
		bool isJump = isHyper ||
			(find(previous->Neighbors().begin(), previous->Neighbors().end(), next)
				!= previous->Neighbors().end());
		bool isWormhole = false;
		if(!((isHyper && hasHyper) || (isJump && hasJump)))
		{
			for(const StellarObject &object : previous->Objects())
				isWormhole |= (object.GetPlanet() && object.GetPlanet()->WormholeDestination(previous) == next);
			if(!isWormhole)
				break;
		}
		
		Point from = Zoom() * (next->Position() + center);
		Point to = Zoom() * (previous->Position() + center);
		Point unit = (from - to).Unit() * 7.;
		from -= unit;
		to += unit;
		
		if(isWormhole)
		{
			// Wormholes cost no fuel to travel through.
		}
		else if(!isHyper)
		{
			if(!escortHasJump)
				escortCapacity = 0.;
			flagshipCapacity -= 200.;
			escortCapacity -= 200.;
		}
		else
		{
			flagshipCapacity -= flagshipJumpFuel;
			escortCapacity -= escortJumpFuel;
		}
		
		Color drawColor = outOfFlagshipFuelRangeColor;
		if(isWormhole)
			drawColor = wormholeColor;
		else if(flagshipCapacity >= 0. && escortCapacity >= 0.)
			drawColor = withinFleetFuelRangeColor;
		else if(flagshipCapacity >= 0. || escortCapacity >= 0.)
			drawColor = defaultColor;
        
		LineShader::Draw(from, to, 3., drawColor);
		
		previous = next;
	}
}



void MapPanel::DrawWormholes() const
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
				const System *next = object.GetPlanet()->WormholeDestination(previous);
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



void MapPanel::DrawLinks() const
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



void MapPanel::DrawSystems() const
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
		else if(system.IsInhabited())
		{
			if(commodity >= SHOW_SPECIAL)
			{
				double value = 0.;
				if(commodity >= 0)
				{
					const Trade::Commodity &com = GameData::Commodities()[commodity];
					value = (2. * (system.Trade(com.name) - com.low))
						/ (com.high - com.low) - 1.;
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
						if(object.GetPlanet())
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
				
				bool hasDominated = false;
				bool canLand = false;
				for(const StellarObject &object : system.Objects())
					if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
					{
						canLand |= object.GetPlanet()->CanLand();
						hasDominated |= GameData::GetPolitics().HasDominated(object.GetPlanet());
					}
				color = ReputationColor(reputation, canLand, hasDominated);
			}
		}
		
		DotShader::Draw(pos, OUTER, INNER, color);
	}
}



void MapPanel::DrawNames() const
{
	// Don't draw if too small.
	if (Zoom() <= 0.5) return;
	
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



void MapPanel::DrawMissions() const
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
		DrawPointer(system, angle[system],
			blink ? black : IsSatisfied(mission) ? currentColor : blockedColor);
		
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



void MapPanel::DrawPointer(const System *system, Angle &angle, const Color &color) const
{
	static const Color black(0., 1.);
	
	angle += Angle(30.);
	Point pos = Zoom() * (system->Position() + center);
	PointerShader::Draw(pos, angle.Unit(), 14., 19., -4., black);
	PointerShader::Draw(pos, angle.Unit(), 8., 15., -6., color);
}
