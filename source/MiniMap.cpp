/* MiniMap.cpp
Copyright (c) 2025 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MiniMap.h"

#include "Color.h"
#include "Command.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "shader/LineShader.h"
#include "Interface.h"
#include "MapPanel.h"
#include "Mission.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "shader/RingShader.h"
#include "Set.h"
#include "Ship.h"
#include "System.h"

#include <set>
#include <vector>

using namespace std;



MiniMap::MiniMap(const PlayerInfo &player)
	: player(player)
{
}



void MiniMap::Fade()
{
	// Only let the minimap linger for 1.5 seconds after a jump has completed.
	displayMinimap = min(displayMinimap, 90);
}



void MiniMap::Step(const shared_ptr<Ship> &flagship)
{
	if(!flagship)
		return;

	// If the flagship is in hyperspace or the player is holding the jump command,
	// determine what system the player is jumping or about to jump into.
	if(flagship->IsEnteringHyperspace() || flagship->Commands().Has(Command::JUMP))
	{
		// Let the minimap linger for 3 seconds after the player hit the jump key.
		displayMinimap = 180;
		// Draw the systems that the player is jumping between on the minimap.
		const System *from = flagship->GetSystem();
		const System *to = flagship->GetTargetSystem();
		if(from && to && from != to)
		{
			current = from;
			target = to;
		}
	}
	// If the player isn't jumping, count down how long the jump 
	else if(displayMinimap > 0)
		--displayMinimap;
	else
	{
		// If the jump has completed, draw the next system in the player's jump
		// plan, or set the minimapSystems to nullptr to only display the current
		// system if the travel plan is empty.
		const vector<const System *> &plan = player.TravelPlan();
		if(plan.empty())
		{
			current = flagship->GetSystem();
			target = nullptr;
		}
		else
		{
			current = flagship->GetSystem();
			target = plan.front();
		}
	}

	// Control the fading in and out of the minimap.
	if(displayMinimap)
	{
		if(displayMinimap < 30 && fadeMinimap)
			--fadeMinimap;
		else if(fadeMinimap < 30)
			++fadeMinimap;
	}
	else
		fadeMinimap = 0;

	// Determine where the minimap should be centered.
	if(target)
		center = .5 * (current->Position() + target->Position());
	else
		center = current->Position();
}



void MiniMap::Draw(int step) const
{
	const Ship *flagship = player.Flagship();
	if(!flagship || !current)
		return;

	Preferences::MinimapDisplay pref = Preferences::GetMinimapDisplay();
	if(pref == Preferences::MinimapDisplay::OFF)
		return;

	float alpha = 1.f;
	if(pref == Preferences::MinimapDisplay::WHEN_JUMPING)
	{
		if(!displayMinimap)
			return;
		alpha = .5f * min(1.f, fadeMinimap / 30.f);
	}

	set<const System *> drawnSystems;
	drawnSystems.insert(current);
	if(target)
		drawnSystems.insert(target);

	const Font &font = FontSet::Get(14);
	Color lineColor(alpha, 0.f);
	Color brightColor(.4f * alpha, 0.f);

	const Point &drawPos = GameData::Interfaces().Get("hud")->GetPoint("mini-map");
	const Set<Color> &colors = GameData::Colors();
	const Color &currentColor = colors.Get("active mission")->Additive(alpha * 2.f);
	const Color &blockedColor = colors.Get("blocked mission")->Additive(alpha * 2.f);
	const Color &waypointColor = colors.Get("waypoint")->Additive(alpha * 2.f);

	const System *flagshipSystem = flagship->GetSystem();
	auto drawSystemLinks = [&](const System &system) -> void {
		static const string UNKNOWN_SYSTEM = "Unexplored System";
		const Government *gov = system.GetGovernment();
		Point from = system.Position() - center + drawPos;
		const string &name = player.KnowsName(system) ? system.DisplayName() : UNKNOWN_SYSTEM;
		font.Draw(name, from + Point(MapPanel::OUTER, -.5 * font.Height()), lineColor);

		// Draw the origin and destination systems, since they
		// might not be linked via hyperspace.
		Color color = Color(.5f * alpha, 0.f);
		if(player.CanView(system) && system.IsInhabited(flagship) && gov)
			color = gov->GetColor().Additive(alpha);
		RingShader::Draw(from, MapPanel::OUTER, MapPanel::INNER, color);

		// Add a circle around the system that the player is currently in.
		if(&system == flagshipSystem)
			RingShader::Draw(from, 11.f, 9.f, brightColor);

		for(const System *link : system.Links())
		{
			// Only draw systems known to be attached to the jump systems.
			if(!player.CanView(system) && !player.CanView(*link))
				continue;

			// Draw the system link. This will double-draw the jump
			// path if it is via hyperlink, to increase brightness.
			Point to = link->Position() - center + drawPos;
			Point unit = (from - to).Unit() * MapPanel::LINK_OFFSET;
			LineShader::Draw(from - unit, to + unit, MapPanel::LINK_WIDTH, lineColor);

			if(drawnSystems.contains(link))
				continue;
			drawnSystems.insert(link);

			gov = link->GetGovernment();
			Color color = Color(.5f * alpha, 0.f);
			if(player.CanView(*link) && link->IsInhabited(flagship) && gov)
				color = gov->GetColor().Additive(alpha);
			RingShader::Draw(to, MapPanel::OUTER, MapPanel::INNER, color);
		}

		unsigned missionCounter = 0;
		for(const Mission &mission : player.Missions())
		{
			if(missionCounter >= MapPanel::MAX_MISSION_POINTERS_DRAWN)
				break;

			if(!mission.IsVisible())
				continue;

			if(mission.Destination()->IsInSystem(&system))
			{
				pair<bool, bool> blink = MapPanel::BlinkMissionIndicator(player, mission, step);
				if(!blink.first)
				{
					bool isSatisfied = mission.IsSatisfied(player) && !mission.IsFailed() && blink.second;
					MapPanel::DrawPointer(from, missionCounter, isSatisfied ? currentColor : blockedColor, false);
				}
				else
					++missionCounter;
			}

			for(const System *waypoint : mission.Waypoints())
			{
				if(missionCounter >= MapPanel::MAX_MISSION_POINTERS_DRAWN)
					break;
				if(waypoint == &system)
					MapPanel::DrawPointer(from, missionCounter, waypointColor, false);
			}
			for(const Planet *stopover : mission.Stopovers())
			{
				if(missionCounter >= MapPanel::MAX_MISSION_POINTERS_DRAWN)
					break;
				if(stopover->IsInSystem(&system))
					MapPanel::DrawPointer(from, missionCounter, waypointColor, false);
			}
			for(const System *mark : mission.MarkedSystems())
			{
				if(missionCounter >= MapPanel::MAX_MISSION_POINTERS_DRAWN)
					break;
				if(mark == &system)
					MapPanel::DrawPointer(from, missionCounter, waypointColor, false);
			}
		}
	};

	drawSystemLinks(*current);
	if(!target)
		return;
	drawSystemLinks(*target);

	// Draw the directional arrow. If this is a normal jump,
	// the stem was already drawn above.
	Point from = current->Position() - center + drawPos;
	Point to = target->Position() - center + drawPos;
	Point unit = (to - from).Unit();
	from += MapPanel::LINK_OFFSET * unit;
	to -= MapPanel::LINK_OFFSET * unit;
	Color bright(2.f * alpha, 0.f);
	// Non-hyperspace jumps are drawn with a dashed directional arrow.
	if(!current->Links().contains(target))
		LineShader::DrawDashed(from, to, unit, MapPanel::LINK_WIDTH, bright, 11., 4.);
	LineShader::Draw(to, to + Angle(-30.).Rotate(unit) * -10., MapPanel::LINK_WIDTH, bright);
	LineShader::Draw(to, to + Angle(30.).Rotate(unit) * -10., MapPanel::LINK_WIDTH, bright);
}
