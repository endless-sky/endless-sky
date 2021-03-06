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

#include "text/alignment.hpp"
#include "Angle.h"
#include "CargoHold.h"
#include "Dialog.h"
#include "FillShader.h"
#include "FogShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
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

#include "gl_header.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>

using namespace std;

namespace {
	// Log how many player ships and stored outfits are in a given system, tracking for
	// ships if they are parked or in-flight.
	//
	// The structure of the map used here is:
	//  - first        -> ships
	//  - first.first  -> ships (in-flight)
	//  - first.second -> ships (parked)
	//  - second       -> outfits (in storage)
	void TallyEscortsAndOutfits(const vector<shared_ptr<Ship>> &escorts,
		const std::map<const Planet *, CargoHold> &outfits,
		map<const System *, pair<pair<int, int>, int>> &locations)
	{
		locations.clear();
		for(const auto &ship : escorts)
		{
			if(ship->IsDestroyed())
				continue;
			if(ship->GetSystem())
			{
				if(!ship->IsParked())
					++locations[ship->GetSystem()].first.first;
				else
					++locations[ship->GetSystem()].first.second;
			}
			// If this ship has no system but has a parent, it is carried (and thus not parked).
			else if(ship->CanBeCarried() && ship->GetParent() && ship->GetParent()->GetSystem())
				++locations[ship->GetParent()->GetSystem()].first.first;
		}
		for(const auto &hold : outfits)
		{
			// Get the system in which the planet storage is located.
			const System* system = hold.first->GetSystem();
			for(const auto &outfit: hold.second.Outfits())
				// Only count a system if it actually stores outfits.
				if(outfit.second)
					locations[system].second += outfit.second;
		}
	}
	
	const Color black(0.f, 1.f);
	const Color red(1.f, 0.f, 0.f, 1.f);
	
	// Hovering an escort pip for this many frames activates the tooltip.
	const int HOVER_TIME = 60;
	// Length in frames of the recentering animation.
	const int RECENTER_TIME = 20;
}

const float MapPanel::OUTER = 6.f;
const float MapPanel::INNER = 3.5f;
const float MapPanel::LINK_WIDTH = 1.2f;
// Draw links only outside the system ring, which has radius MapPanel::OUTER.
const float MapPanel::LINK_OFFSET = 7.f;



MapPanel::MapPanel(PlayerInfo &player, int commodity, const System *special)
	: player(player), distance(player),
	playerSystem(*player.GetSystem()),
	selectedSystem(special ? special : player.GetSystem()),
	specialSystem(special),
	playerJumpDistance(System::DEFAULT_NEIGHBOR_DISTANCE),
	commodity(commodity)
{
	SetIsFullScreen(true);
	SetInterruptible(false);
	// Recalculate the fog each time the map is opened, just in case the player
	// bought a map since the last time they viewed the map.
	FogShader::Redraw();
	
	// Recalculate escort positions every time the map is opened, as they may
	// be changing systems even if the player does not.
	// The player cannot toggle any preferences without closing the map panel.
	if(Preferences::Has("Show escort systems on map"))
		TallyEscortsAndOutfits(player.Ships(), player.PlanetaryStorage(), escortSystems);
	
	// Initialize a centered tooltip.
	hoverText.SetFont(FontSet::Get(14));
	hoverText.SetWrapWidth(150);
	hoverText.SetAlignment(Alignment::LEFT);
	
	// Find out how far the player is able to jump. The range of the system
	// takes priority over the range of the player's flagship.
	double systemRange = playerSystem.JumpRange();
	double playerRange = player.Flagship() ? player.Flagship()->JumpRange() : 0.;
	if(systemRange || playerRange)
		playerJumpDistance = systemRange ? systemRange : playerRange;
	
	CenterOnSystem(selectedSystem, true);
}



void MapPanel::Step()
{
	if(recentering > 0)
	{
		double step = (recentering - .5) / RECENTER_TIME;
		// Interpolate with the smoothstep function, 3x^2 - 2x^3. Its derivative
		// gives the fraction of the distance to move at each time step:
		center += recenterVector * (step * (1. - step) * (6. / RECENTER_TIME));
		--recentering;
	}
}



void MapPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	for(const auto &it : GameData::Galaxies())
		SpriteShader::Draw(it.second.GetSprite(), Zoom() * (center + it.second.Position()), Zoom());
	
	if(Preferences::Has("Hide unexplored map regions"))
		FogShader::Draw(center, Zoom(), player);
	
	// Draw the "visible range" circle around your current location.
	Color dimColor(.1f, 0.f);
	RingShader::Draw(Zoom() * (playerSystem.Position() + center),
		(System::DEFAULT_NEIGHBOR_DISTANCE + .5) * Zoom(), (System::DEFAULT_NEIGHBOR_DISTANCE - .5) * Zoom(), dimColor);
	// Draw the jump range circle around your current location if it is different than the
	// visible range.
	if(playerJumpDistance != System::DEFAULT_NEIGHBOR_DISTANCE)
		RingShader::Draw(Zoom() * (playerSystem.Position() + center),
			(playerJumpDistance + .5) * Zoom(), (playerJumpDistance - .5) * Zoom(), dimColor);
	
	Color brightColor(.4f, 0.f);
	RingShader::Draw(Zoom() * (selectedSystem->Position() + center),
		11.f, 9.f, brightColor);
	
	// Advance a "blink" timer.
	++step;
	// Update the tooltip timer [0-60].
	hoverCount += hoverSystem ? (hoverCount < HOVER_TIME) : (hoverCount ? -1 : 0);
	
	DrawWormholes();
	DrawTravelPlan();
	DrawEscorts();
	DrawLinks();
	DrawSystems();
	DrawNames();
	DrawMissions();
	DrawTooltips();
	
	if(selectedSystem != &playerSystem && !distance.HasRoute(selectedSystem))
	{
		static const string UNAVAILABLE = "You have no available route to this system.";
		static const string UNKNOWN = "You have not yet mapped a route to this system.";
		const Font &font = FontSet::Get(18);
		
		const string &message = player.HasVisited(*selectedSystem) ? UNAVAILABLE : UNKNOWN;
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
	const Interface *mapInterface = GameData::Interfaces().Get("map");
	if(player.MapZoom() >= static_cast<int>(mapInterface->GetValue("max zoom")))
		info.SetCondition("max zoom");
	if(player.MapZoom() <= static_cast<int>(mapInterface->GetValue("min zoom")))
		info.SetCondition("min zoom");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info, this);
}



void MapPanel::DrawMiniMap(const PlayerInfo &player, float alpha, const System *const jump[2], int step)
{
	const Font &font = FontSet::Get(14);
	Color lineColor(alpha, 0.f);
	Point center = .5 * (jump[0]->Position() + jump[1]->Position());
	const Point &drawPos = GameData::Interfaces().Get("hud")->GetPoint("mini-map");
	set<const System *> drawnSystems = { jump[0], jump[1] };
	bool isLink = jump[0]->Links().count(jump[1]);
	
	const Set<Color> &colors = GameData::Colors();
	const Color &currentColor = colors.Get("active mission")->Additive(alpha * 2.f);
	const Color &blockedColor = colors.Get("blocked mission")->Additive(alpha * 2.f);
	const Color &waypointColor = colors.Get("waypoint")->Additive(alpha * 2.f);
	
	const Ship *flagship = player.Flagship();
	for(int i = 0; i < 2; ++i)
	{
		static const string UNKNOWN_SYSTEM = "Unexplored System";
		const System &system = *jump[i];
		const Government *gov = system.GetGovernment();
		Point from = system.Position() - center + drawPos;
		const string &name = player.KnowsName(system) ? system.Name() : UNKNOWN_SYSTEM;
		font.Draw(name, from + Point(OUTER, -.5 * font.Height()), lineColor);
		
		// Draw the origin and destination systems, since they
		// might not be linked via hyperspace.
		Color color = Color(.5f * alpha, 0.f);
		if(player.HasVisited(system) && system.IsInhabited(flagship) && gov)
			color = Color(
				alpha * gov->GetColor().Get()[0],
				alpha * gov->GetColor().Get()[1],
				alpha * gov->GetColor().Get()[2], 0.f);
		RingShader::Draw(from, OUTER, INNER, color);
		
		for(const System *link : system.Links())
		{
			// Only draw systems known to be attached to the jump systems.
			if(!player.HasVisited(system) && !player.HasVisited(*link))
				continue;
			
			// Draw the system link. This will double-draw the jump
			// path if it is via hyperlink, to increase brightness.
			Point to = link->Position() - center + drawPos;
			Point unit = (from - to).Unit() * LINK_OFFSET;
			LineShader::Draw(from - unit, to + unit, LINK_WIDTH, lineColor);
			
			if(drawnSystems.count(link))
				continue;
			drawnSystems.insert(link);
			
			gov = link->GetGovernment();
			Color color = Color(.5f * alpha, 0.f);
			if(player.HasVisited(*link) && link->IsInhabited(flagship) && gov)
				color = Color(
					alpha * gov->GetColor().Get()[0],
					alpha * gov->GetColor().Get()[1],
					alpha * gov->GetColor().Get()[2], 0.f);
			RingShader::Draw(to, OUTER, INNER, color);
		}
		
		Angle angle;
		for(const Mission &mission : player.Missions())
		{
			if(!mission.IsVisible())
				continue;
			
			if(mission.Destination()->IsInSystem(&system))
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
				if(waypoint == &system)
					DrawPointer(from, angle, waypointColor, false);
			for(const Planet *stopover : mission.Stopovers())
				if(stopover->IsInSystem(&system))
					DrawPointer(from, angle, waypointColor, false);
		}
	}
	
	// Draw the rest of the directional arrow. If this is a normal jump,
	// the stem was already drawn above.
	Point from = jump[0]->Position() - center + drawPos;
	Point to = jump[1]->Position() - center + drawPos;
	Point unit = (to - from).Unit();
	from += LINK_OFFSET * unit;
	to -= LINK_OFFSET * unit;
	Color bright(2.f * alpha, 0.f);
	// Non-hyperspace jumps are drawn with a dashed directional arrow.
	if(!isLink)
	{
		double length = (to - from).Length();
		int segments = static_cast<int>(length / 15.);
		for(int i = 0; i < segments; ++i)
			LineShader::Draw(
				from + unit * ((i * length) / segments + 2.),
				from + unit * (((i + 1) * length) / segments - 2.),
				LINK_WIDTH, bright);
	}
	LineShader::Draw(to, to + Angle(-30.).Rotate(unit) * -10., LINK_WIDTH, bright);
	LineShader::Draw(to, to + Angle(30.).Rotate(unit) * -10., LINK_WIDTH, bright);
}



bool MapPanel::AllowFastForward() const
{
	return true;
}



bool MapPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	const Interface *mapInterface = GameData::Interfaces().Get("map");
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
	else if(key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS)
		player.SetMapZoom(min(static_cast<int>(mapInterface->GetValue("max zoom")), player.MapZoom() + 1));
	else if(key == SDLK_MINUS || key == SDLK_KP_MINUS)
		player.SetMapZoom(max(static_cast<int>(mapInterface->GetValue("min zoom")), player.MapZoom() - 1));
	else
		return false;
	
	return true;
}



bool MapPanel::Click(int x, int y, int clicks)
{
	// Figure out if a system was clicked on.
	Point click = Point(x, y) / Zoom() - center;
	for(const auto &it : GameData::Systems())
		if(it.second.IsValid() && click.Distance(it.second.Position()) < 10.
				&& (player.HasSeen(it.second) || &it.second == specialSystem))
		{
			Select(&it.second);
			break;
		}
	
	return true;
}



// If the mouse has moved near a known system that contains escorts, track the dwell time.
bool MapPanel::Hover(int x, int y)
{
	if(escortSystems.empty())
		return true;
	
	// Map from screen coordinates into game coordinates.
	Point pos = Point(x, y) / Zoom() - center;
	double maxDistance = 2 * OUTER / Zoom();
	
	// Were we already hovering near an escort's system?
	if(hoverSystem)
	{
		// Is the new mouse position still near it?
		if(pos.Distance(hoverSystem->Position()) <= maxDistance)
			return true;
		
		hoverSystem = nullptr;
		tooltip.clear();
	}
	
	// Check if the new position supports a tooltip.
	for(const auto &squad : escortSystems)
	{
		const System *system = squad.first;
		if(pos.Distance(system->Position()) < maxDistance
				&& (player.HasSeen(*system) || system == specialSystem))
		{
			// Start tracking this system.
			hoverSystem = system;
			break;
		}
	}
	return true;
}



bool MapPanel::Drag(double dx, double dy)
{
	center += Point(dx, dy) / Zoom();
	recentering = 0;
	
	return true;
}



bool MapPanel::Scroll(double dx, double dy)
{
	// The mouse should be pointing to the same map position before and after zooming.
	Point mouse = UI::GetMouse();
	Point anchor = mouse / Zoom() - center;
	const Interface *mapInterface = GameData::Interfaces().Get("map");
	if(dy > 0.)
		player.SetMapZoom(min(static_cast<int>(mapInterface->GetValue("max zoom")), player.MapZoom() + 1));
	else if(dy < 0.)
		player.SetMapZoom(max(static_cast<int>(mapInterface->GetValue("min zoom")), player.MapZoom() - 1));
	
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
		.6f * government->GetColor().Get()[0],
		.6f * government->GetColor().Get()[1],
		.6f * government->GetColor().Get()[2],
		.4f);
}



Color MapPanel::UninhabitedColor()
{
	return GovernmentColor(GameData::Governments().Get("Uninhabited"));
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
	Ship *flagship = player.Flagship();
	if(!flagship || (!plan.empty() && system == plan.front()))
		return;
	
	bool isJumping = flagship->IsEnteringHyperspace();
	const System *source = isJumping ? flagship->GetTargetSystem() : &playerSystem;
	
	auto mod = SDL_GetModState();
	// TODO: Whoever called Select should tell us what to do with this system vis-a-vis the travel plan, rather than
	// possibly manipulating it both here and there. Or, we entirely separate Select from travel plan modifications.
	bool shift = (mod & KMOD_SHIFT) && !plan.empty();
	if(mod & KMOD_CTRL)
		return;
	else if(system == source && !shift)
	{
		plan.clear();
		if(!isJumping)
			flagship->SetTargetSystem(nullptr);
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
			flagship->SetTargetSystem(nullptr);
		
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
		if(it.second.IsValid() && player.HasVisited(it.second))
		{
			int index = Search(it.first, name);
			if(index >= 0 && index < bestIndex)
			{
				bestIndex = index;
				selectedSystem = &it.second;
				CenterOnSystem(selectedSystem);
				if(!index)
				{
					selectedPlanet = nullptr;
					return;
				}
			}
		}
	for(const auto &it : GameData::Planets())
		if(it.second.IsValid() && player.HasVisited(*it.second.GetSystem()))
		{
			int index = Search(it.first, name);
			if(index >= 0 && index < bestIndex)
			{
				bestIndex = index;
				selectedSystem = it.second.GetSystem();
				CenterOnSystem(selectedSystem);
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



void MapPanel::CenterOnSystem(const System *system, bool immediate)
{
	if(immediate)
		center = -system->Position();
	else
	{
		recenterVector = -system->Position() - center;
		recentering = RECENTER_TIME;
	}
}



// Cache the map layout, so it doesn't have to be re-calculated every frame.
// The node cache must be updated when the coloring mode changes.
void MapPanel::UpdateCache()
{
	// Remember which commodity the cached systems are colored by.
	cachedCommodity = commodity;
	nodes.clear();
	
	// Draw the circles for the systems, colored based on the selected criterion,
	// which may be government, services, or commodity prices.
	const Color &closeNameColor = *GameData::Colors().Get("map name");
	const Color &farNameColor = closeNameColor.Transparent(.5);
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		// Ignore systems which have been referred to, but not actually defined.
		if(!system.IsValid())
			continue;
		// Ignore systems the player has never seen, unless they have a pending mission that lets them see it.
		if(!player.HasSeen(system) && &system != specialSystem)
			continue;
		
		Color color = UninhabitedColor();
		if(!player.HasVisited(system))
			color = UnexploredColor();
		else if(system.IsInhabited(player.Flagship()) || commodity == SHOW_SPECIAL || commodity == SHOW_VISITED)
		{
			if(commodity >= SHOW_SPECIAL)
			{
				double value = 0.;
				bool colorSystem = true;
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
					colorSystem = false;
					for(const StellarObject &object : system.Objects())
						if(object.GetPlanet() && !object.GetPlanet()->IsWormhole()
							&& object.GetPlanet()->IsAccessible(player.Flagship()))
						{
							bool visited = player.HasVisited(*object.GetPlanet());
							all &= visited;
							some |= visited;
							colorSystem = true;
						}
					value = -1 + some + all;
				}
				else
					value = SystemValue(&system);
				
				if(colorSystem)
					color = MapColor(value);
			}
			else if(commodity == SHOW_GOVERNMENT)
			{
				const Government *gov = system.GetGovernment();
				color = GovernmentColor(gov);
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
		
		nodes.emplace_back(system.Position(), color,
			player.KnowsName(system) ? system.Name() : "",
			(&system == &playerSystem) ? closeNameColor : farNameColor,
			player.HasVisited(system) ? system.GetGovernment() : nullptr);
	}
	
	// Now, update the cache of the links.
	links.clear();
	
	// The link color depends on whether it's connected to the current system or not.
	const Color &closeColor = *GameData::Colors().Get("map link");
	const Color &farColor = closeColor.Transparent(.5);
	for(const auto &it : GameData::Systems())
	{
		const System *system = &it.second;
		if(!system->IsValid() || !player.HasSeen(*system))
			continue;
		
		for(const System *link : system->Links())
			if(link < system || !player.HasSeen(*link))
			{
				// Only draw links between two systems if one of the two is
				// visited. Also, avoid drawing twice by only drawing in the
				// direction of increasing pointer values.
				if((!player.HasVisited(*system) && !player.HasVisited(*link)) || !link->IsValid())
					continue;
				
				bool isClose = (system == &playerSystem || link == &playerSystem);
				links.emplace_back(system->Position(), link->Position(), isClose ? closeColor : farColor);
			}
	}
}



void MapPanel::DrawTravelPlan()
{
	const Set<Color> &colors = GameData::Colors();
	const Color &defaultColor = *colors.Get("map travel ok flagship");
	const Color &outOfFlagshipFuelRangeColor = *colors.Get("map travel ok none");
	const Color &withinFleetFuelRangeColor = *colors.Get("map travel ok fleet");
	const Color &wormholeColor = *colors.Get("map used wormhole");
	
	// At each point in the path, keep track of how many ships in the
	// fleet are able to make it this far.
	const Ship *flagship = player.Flagship();
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
	
	const System *previous = &playerSystem;
	double jumpRange = flagship->JumpRange();
	for(int i = player.TravelPlan().size() - 1; i >= 0; --i)
	{
		const System *next = player.TravelPlan()[i];
		bool isHyper = previous->Links().count(next);
		bool isJump = !isHyper && previous->JumpNeighbors(jumpRange).count(next);
		bool systemJumpRange = previous->JumpRange() > 0.;
		bool isWormhole = false;
		for(const StellarObject &object : previous->Objects())
			isWormhole |= (object.GetPlanet() && player.HasVisited(*object.GetPlanet())
				&& !object.GetPlanet()->Description().empty()
				&& player.HasVisited(*previous) && player.HasVisited(*next)
				&& object.GetPlanet()->WormholeDestination(previous) == next);
		
		if(!isHyper && !isJump && !isWormhole)
			break;
		
		double jumpDistance = previous->Position().Distance(next->Position());
		// Wormholes cost nothing to go through. If this is not a wormhole,
		// check how much fuel every ship will expend to go through it.
		if(!isWormhole)
			for(auto &it : fuel)
				if(it.second >= 0.)
				{
					double cost = isJump ? it.first->JumpDriveFuel(systemJumpRange ? 0. : jumpDistance) : it.first->HyperdriveFuel();
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
		Point unit = (from - to).Unit() * LINK_OFFSET;
		LineShader::Draw(from - unit, to + unit, 3.f, drawColor);
		
		previous = next;
	}
}



// Communicate the location of non-destroyed, player-owned ships.
void MapPanel::DrawEscorts()
{
	if(escortSystems.empty())
		return;
	
	// Fill in the center of any system containing the player's ships, if the
	// player knows about that system (since escorts may use unknown routes).
	const Color &active = *GameData::Colors().Get("map link");
	const Color &parked = *GameData::Colors().Get("dim");
	double zoom = Zoom();
	for(const auto &squad : escortSystems)
		if(player.HasSeen(*squad.first) || squad.first == specialSystem)
		{
			Point pos = zoom * (squad.first->Position() + center);
			
			// Active and parked ships are drawn/indicated by a ring in the center.
			if(squad.second.first.first || squad.second.first.second)
				RingShader::Draw(pos, INNER - 1.f, 0.f, squad.second.first.first ? active : parked);
			
			if(squad.second.second)
				// Stored outfits are drawn/indicated by 8 short rays out of the system center.
				for(int i = 0; i < 8; ++i)
				{
					// Starting at 7.5 degrees to intentionally mis-align with mission pointers.
					Angle angle = Angle(7.5f + 45.f * i);
					Point from = pos + angle.Unit() * OUTER;
					Point to = from + angle.Unit() * 4.f;
					LineShader::Draw(from, to, 2.f, active);
				}
		}
}



void MapPanel::DrawWormholes()
{
	// Keep track of what arrows and links need to be drawn.
	set<pair<const System *, const System *>> arrowsToDraw;
	
	// Avoid iterating each StellarObject in every system by iterating over planets instead. A
	// system can host more than one set of wormholes (e.g. Cardea), and some wormholes may even
	// share a link vector. If a wormhole's planet has no description, no link will be drawn.
	for(auto &&it : GameData::Planets())
	{
		const Planet &p = it.second;
		if(!p.IsValid() || !p.IsWormhole() || !player.HasVisited(p) || p.Description().empty())
			continue;
		
		const vector<const System *> &waypoints = p.WormholeSystems();
		const System *from = waypoints.back();
		for(const System *to : waypoints)
		{
			if(player.HasVisited(*from) && player.HasVisited(*to))
				arrowsToDraw.emplace(from, to);
			
			from = to;
		}
	}
	
	const Color &wormholeDim = *GameData::Colors().Get("map unused wormhole");
	const Color &arrowColor = *GameData::Colors().Get("map used wormhole");
	static const double ARROW_LENGTH = 4.;
	static const double ARROW_RATIO = .3;
	static const Angle LEFT(30.);
	static const Angle RIGHT(-30.);
	const double zoom = Zoom();
	
	for(const pair<const System *, const System *> &link : arrowsToDraw)
	{
		// Compute the start and end positions of the wormhole link.
		Point from = zoom * (link.first->Position() + center);
		Point to = zoom * (link.second->Position() + center);
		Point offset = (from - to).Unit() * LINK_OFFSET;
		from -= offset;
		to += offset;
		
		// If an arrow is being drawn, the link will always be drawn too. Draw
		// the link only for the first instance of it in this set.
		if(link.first < link.second || !arrowsToDraw.count(make_pair(link.second, link.first)))
			LineShader::Draw(from, to, LINK_WIDTH, wormholeDim);
		
		// Compute the start and end positions of the arrow edges.
		Point arrowStem = zoom * ARROW_LENGTH * offset;
		Point arrowLeft = arrowStem - ARROW_RATIO * LEFT.Rotate(arrowStem);
		Point arrowRight = arrowStem - ARROW_RATIO * RIGHT.Rotate(arrowStem);
		
		// Draw the arrowhead.
		Point fromTip = from - arrowStem;
		LineShader::Draw(from, fromTip, LINK_WIDTH, arrowColor);
		LineShader::Draw(from - arrowLeft, fromTip, LINK_WIDTH, arrowColor);
		LineShader::Draw(from - arrowRight, fromTip, LINK_WIDTH, arrowColor);
	}
}



void MapPanel::DrawLinks()
{
	double zoom = Zoom();
	for(const Link &link : links)
	{
		Point from = zoom * (link.start + center);
		Point to = zoom * (link.end + center);
		Point unit = (from - to).Unit() * LINK_OFFSET;
		from -= unit;
		to += unit;
		
		LineShader::Draw(from, to, LINK_WIDTH, link.color);
	}
}



void MapPanel::DrawSystems()
{
	if(commodity != cachedCommodity)
		UpdateCache();
	
	// If coloring by government, we need to keep track of which ones are the
	// closest to the center of the window because those will be the ones that
	// are shown in the map key.
	if(commodity == SHOW_GOVERNMENT)
		closeGovernments.clear();
	
	// Draw the circles for the systems.
	double zoom = Zoom();
	for(const Node &node : nodes)
	{
		Point pos = zoom * (node.position + center);
		RingShader::Draw(pos, OUTER, INNER, node.color);
		
		if(commodity == SHOW_GOVERNMENT && node.government && node.government->GetName() != "Uninhabited")
		{
			// For every government that is drawn, keep track of how close it
			// is to the center of the view. The four closest governments
			// will be displayed in the key.
			double distance = pos.Length();
			auto it = closeGovernments.find(node.government);
			if(it == closeGovernments.end())
				closeGovernments[node.government] = distance;
			else
				it->second = min(it->second, distance);
		}
	}
}



void MapPanel::DrawNames()
{
	// Don't draw if too small.
	double zoom = Zoom();
	if(zoom <= 0.5)
		return;
	
	// Draw names for all systems you have visited.
	bool useBigFont = (zoom > 2.);
	const Font &font = FontSet::Get(useBigFont ? 18 : 14);
	Point offset(useBigFont ? 8. : 6., -.5 * font.Height());
	for(const Node &node : nodes)
		font.Draw(node.name, zoom * (node.position + center) + offset, node.nameColor);
}



void MapPanel::DrawMissions()
{
	// Draw a pointer for each active or available mission.
	map<const System *, Angle> angle;
	
	const Set<Color> &colors = GameData::Colors();
	const Color &availableColor = *colors.Get("available job");
	const Color &unavailableColor = *colors.Get("unavailable job");
	const Color &currentColor = *colors.Get("active mission");
	const Color &blockedColor = *colors.Get("blocked mission");
	const Color &specialColor = *colors.Get("special mission");
	const Color &waypointColor = *colors.Get("waypoint");
	for(const Mission &mission : player.AvailableJobs())
	{
		const System *system = mission.Destination()->GetSystem();
		DrawPointer(system, angle[system], mission.HasSpace(player) ? availableColor : unavailableColor);
	}
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
		PointerShader::Draw(pos, a.Unit(), 20.f, 27.f, -4.f, black);
		PointerShader::Draw(pos, a.Unit(), 11.5f, 21.5f, -6.f, specialColor);
	}
}



void MapPanel::DrawTooltips()
{
	if(!hoverSystem || hoverCount < HOVER_TIME)
		return;
	
	// Create the tooltip text.
	if(tooltip.empty())
	{
		pair<pair<int, int>, int> t = escortSystems.at(hoverSystem);
		if(hoverSystem == &playerSystem)
		{
			--t.first.first;
			if(t.first.first || t.first.second || t.second)
				tooltip = "You are here, with:\n";
			else
				tooltip = "You are here.";
		}
		// If you have both active and parked escorts, call the active ones
		// "active escorts." Otherwise, just call them "escorts."
		if(t.first.first && t.first.second)
			tooltip += to_string(t.first.first) + (t.first.first == 1 ? " active escort\n" : " active escorts\n");
		else if(t.first.first)
			tooltip += to_string(t.first.first) + (t.first.first == 1 ? " escort" : " escorts");
		if(t.first.second)
			tooltip += to_string(t.first.second) + (t.first.second == 1 ? " parked escort" : " parked escorts");
		if(t.second)
		{
			if(t.first.first || t.first.second)
				tooltip += "\n";
			tooltip += to_string(t.second) + (t.second == 1 ? " stored outfit" : " stored outfits");
		}
		
		hoverText.Wrap(tooltip);
	}
	if(!tooltip.empty())
	{
		// Add 10px margin to all sides of the text.
		Point size(hoverText.WrapWidth(), hoverText.Height() - hoverText.ParagraphBreak());
		size += Point(20., 20.);
		Point topLeft = (hoverSystem->Position() + center) * Zoom();
		// Do not overflow the screen dimensions.
		if(topLeft.X() + size.X() > Screen::Right())
			topLeft.X() -= size.X();
		if(topLeft.Y() + size.Y() > Screen::Bottom())
			topLeft.Y() -= size.Y();
		// Draw the background fill and the tooltip text.
		FillShader::Fill(topLeft + .5 * size, size, *GameData::Colors().Get("tooltip background"));
		hoverText.Draw(topLeft + Point(10., 10.), *GameData::Colors().Get("medium"));
	}
}



void MapPanel::DrawPointer(const System *system, Angle &angle, const Color &color, bool bigger)
{
	DrawPointer(Zoom() * (system->Position() + center), angle, color, true, bigger);
}



void MapPanel::DrawPointer(Point position, Angle &angle, const Color &color, bool drawBack, bool bigger)
{
	angle += Angle(30.);
	if(drawBack)
		PointerShader::Draw(position, angle.Unit(), 14.f + bigger, 19.f + 2 * bigger, -4.f, black);
	PointerShader::Draw(position, angle.Unit(), 8.f + bigger, 15.f + 2 * bigger, -6.f, color);
}
