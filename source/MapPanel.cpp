/* MapPanel.cpp
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

#include "MapPanel.h"

#include "text/Alignment.h"
#include "Angle.h"
#include "audio/Audio.h"
#include "shader/BatchDrawList.h"
#include "CargoHold.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "shader/FogShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "Galaxy.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "shader/LineShader.h"
#include "MapDetailPanel.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "Mission.h"
#include "MissionPanel.h"
#include "pi.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "shader/PointerShader.h"
#include "Politics.h"
#include "Preferences.h"
#include "shader/RingShader.h"
#include "RoutePlan.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipJumpNavigation.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "Trade.h"
#include "text/Truncate.h"
#include "UI.h"
#include "Wormhole.h"

#include "opengl.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>

using namespace std;

namespace {
	const string SHOW_ESCORT_SYSTEMS = "Show escort systems on map";
	const string SHOW_STORED_OUTFITS = "Show stored outfits on map";
	const double MISSION_POINTERS_ANGLE_DELTA = 30.;
	const int MAX_STARS = 5;

	// Class to track per system how many pointers are drawn and still
	// need to be drawn.
	class PointerDrawCount {
	public:
		// Calculate and check the most number of pointer positions that should be available for active missions.
		// This can be up to half the maximum number of pointers that can be drawn.
		void Reserve();

		unsigned MaximumActive() const;

	public:
		// Amount of systems already drawn.
		unsigned drawn = 0;
		unsigned available = 0;
		unsigned unavailable = 0;

	private:
		unsigned maximumActive = MapPanel::MAX_MISSION_POINTERS_DRAWN;
	};

	void PointerDrawCount::Reserve()
	{
		maximumActive = max(MapPanel::MAX_MISSION_POINTERS_DRAWN / 2,
			MapPanel::MAX_MISSION_POINTERS_DRAWN - (available + unavailable));
	}

	unsigned PointerDrawCount::MaximumActive() const
	{
		return maximumActive;
	}


	// Struct for storing the ends of wormhole links and their colors.
	struct WormholeArrow {
		WormholeArrow() = default;
		WormholeArrow(const System *from, const System *to, const Color *color)
		: from(from), to(to), color(color) {}
		const System *from = nullptr;
		const System *to = nullptr;
		const Color *color = nullptr;
	};


	// Log how many player ships are in a given system, tracking whether
	// they are parked or in-flight.
	void TallyEscorts(const vector<shared_ptr<Ship>> &escorts,
		map<const System *, MapPanel::SystemTooltipData> &locations)
	{
		for(const auto &ship : escorts)
		{
			if(ship->IsDestroyed())
				continue;
			if(ship->GetSystem())
			{
				if(!ship->IsParked())
					++locations[ship->GetSystem()].activeShips;
				else
					++locations[ship->GetSystem()].parkedShips;
			}
			// If this ship has no system but has a parent, it is carried (and thus not parked).
			else if(ship->CanBeCarried() && ship->GetParent() && ship->GetParent()->GetSystem())
				++locations[ship->GetParent()->GetSystem()].activeShips;
		}
	}

	// Log how many stored outfits are in a given system.
	void TallyOutfits(const map<const Planet *, CargoHold> &outfits,
		map<const System *, MapPanel::SystemTooltipData> &locations)
	{
		for(const auto &hold : outfits)
		{
			// Get the system in which the planet storage is located.
			const Planet *planet = hold.first;
			const System *system = planet->GetSystem();
			// Skip outfits stored on planets without a system or an outfitter.
			if(!system || !planet->HasOutfitter())
				continue;

			for(const auto &outfit : hold.second.Outfits())
				// Only count a system if it actually stores outfits.
				if(outfit.second)
					locations[system].outfits[planet] += outfit.second;
		}
	}

	const Color black(0.f, 1.f);

	// Length in frames of the recentering animation.
	const int RECENTER_TIME = 20;

	bool HasMultipleLandablePlanets(const System &system)
	{
		const Planet *firstPlanet = nullptr;
		for(auto &&stellarObject : system.Objects())
			if(stellarObject.HasValidPlanet() && stellarObject.HasSprite() && !stellarObject.GetPlanet()->IsWormhole())
			{
				// We can return true once we found 2 different landable planets.
				if(!firstPlanet)
					firstPlanet = stellarObject.GetPlanet();
				else if(firstPlanet != stellarObject.GetPlanet())
					return true;
			}

		return false;
	}

	// Return total value of raid fleet (if any) and 60 frames worth of system danger.
	double DangerFleetTotal(const PlayerInfo &player, const System &system, const bool withRaids)
	{
		double danger = system.Danger() * 60.;
		if(withRaids)
			for(const auto &raidFleet : system.GetGovernment()->RaidFleets())
				danger += 10. * player.RaidFleetAttraction(raidFleet, &system) *
					raidFleet.GetFleet()->Strength();
		return danger;
	}

}

const unsigned MapPanel::MAX_MISSION_POINTERS_DRAWN = 12;
const float MapPanel::OUTER = 6.f;
const float MapPanel::INNER = 3.5f;
const float MapPanel::LINK_WIDTH = 1.2f;
// Draw links only outside the system ring, which has radius MapPanel::OUTER.
const float MapPanel::LINK_OFFSET = 7.f;



void MapPanel::DrawPointer(Point position, unsigned &systemCount, const Color &color, bool drawBack, bool bigger)
{
	if(++systemCount > MAX_MISSION_POINTERS_DRAWN)
		return;
	Angle angle = Angle(MISSION_POINTERS_ANGLE_DELTA * systemCount);
	if(drawBack)
		PointerShader::Draw(position, angle.Unit(), 14.f + bigger, 19.f + 2 * bigger, -4.f, black);
	PointerShader::Draw(position, angle.Unit(), 8.f + bigger, 15.f + 2 * bigger, -6.f, color);
}



pair<bool, bool> MapPanel::BlinkMissionIndicator(const PlayerInfo &player, const Mission &mission, int step)
{
	bool blink = false;
	int daysLeft = 1;
	if(mission.Deadline())
	{
		daysLeft = player.RemainingDeadline(mission);
		int blinkFactor = min(6, max(1, daysLeft));
		blink = (step % (10 * blinkFactor) > 5 * blinkFactor);
	}
	return pair<bool, bool>(blink, daysLeft > 0);
}



MapPanel::MapPanel(PlayerInfo &player, int commodity, const System *special, bool fromMission)
	: player(player), distance(player),
	playerSystem(*player.GetSystem()),
	selectedSystem(special ? special : player.GetSystem()),
	specialSystem(special),
	playerJumpDistance(System::DEFAULT_NEIGHBOR_DISTANCE),
	commodity(commodity),
	tooltip(170, Alignment::LEFT, Tooltip::Direction::DOWN_RIGHT, Tooltip::Corner::BOTTOM_RIGHT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium")),
	fromMission(fromMission)
{
	Audio::Pause();
	UI::PlaySound(UI::UISound::SOFT);
	SetIsFullScreen(true);
	SetInterruptible(false);
	zoom.Set(player.MapZoom(), 0);
	// Recalculate the fog each time the map is opened, just in case the player
	// bought a map since the last time they viewed the map.
	FogShader::Redraw();

	// Recalculate escort positions every time the map is opened, as they may
	// be changing systems even if the player does not.
	// The player cannot toggle any preferences without closing the map panel.
	if(Preferences::Has(SHOW_ESCORT_SYSTEMS) || Preferences::Has(SHOW_STORED_OUTFITS))
	{
		escortSystems.clear();
		if(Preferences::Has(SHOW_ESCORT_SYSTEMS))
			TallyEscorts(player.Ships(), escortSystems);
		if(Preferences::Has(SHOW_STORED_OUTFITS))
			TallyOutfits(player.PlanetaryStorage(), escortSystems);
	}

	// Find out how far the player is able to jump. The range of the system
	// takes priority over the range of the player's flagship.
	double systemRange = playerSystem.JumpRange();
	double playerRange = player.Flagship() ? player.Flagship()->JumpNavigation().JumpRange() : 0.;
	if(systemRange || playerRange)
		playerJumpDistance = systemRange ? systemRange : playerRange;

	// Recalculate any mission deadlines if the player is landed in case
	// changes to the player's flagship have changed the deadline calculations.
	// If the player is not landed, then the deadlines will have already been
	// recalculated on the day change.
	if(player.GetPlanet())
		player.CacheMissionInformation(true);

	CenterOnSystem(selectedSystem, true);
}



MapPanel::~MapPanel()
{
	Audio::Resume();
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

	// The mouse should be pointing to the same map position before and after zooming.
	bool needsRecenter = !zoom.IsAnimationDone();
	Point mouse, anchor;
	if(needsRecenter)
	{
		mouse = UI::GetMouse();
		anchor = mouse / Zoom() - center;
	}

	zoom.Step();

	// Now, Zoom() has changed (unless at one of the limits). But, we still want
	// anchor to be the same, so:
	if(needsRecenter)
		center = mouse / Zoom() - anchor;
}



void MapPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);

	for(const auto &it : GameData::Galaxies())
		SpriteShader::Draw(it.second.GetSprite(), Zoom() * (center + it.second.Position()), Zoom());

	if(Preferences::Has("Hide unexplored map regions"))
		FogShader::Draw(center, Zoom(), player);

	// Draw the "visible range" circle around your current location.
	const Color &viewRangeColor = *GameData::Colors().Get("map view range color");
	RingShader::Draw(Zoom() * (playerSystem.Position() + center),
		System::DEFAULT_NEIGHBOR_DISTANCE * Zoom(), 2.0f, 1.0f, viewRangeColor);
	// Draw the jump range circle around your current location if it is different than the
	// visible range.
	const Color &jumpRangeColor = *GameData::Colors().Get("map jump range color");
	if(playerJumpDistance != System::DEFAULT_NEIGHBOR_DISTANCE)
		RingShader::Draw(Zoom() * (playerSystem.Position() + center),
			(playerJumpDistance + .5) * Zoom(), (playerJumpDistance - .5) * Zoom(), jumpRangeColor);

	// Draw a circle around the selected system.
	Color brightColor(.4f, 0.f);
	RingShader::Draw(Zoom() * (selectedSystem->Position() + center),
		11.f, 9.f, brightColor);

	// Advance a "blink" timer.
	++step;
	// Update the tooltip timer.
	if(hoverSystem)
		tooltip.IncrementCount();
	else
		tooltip.DecrementCount();

	DrawWormholes();
	DrawTravelPlan();
	DrawEscorts();
	DrawLinks();
	DrawSystems();
	DrawNames();
	DrawMissions();
}



void MapPanel::FinishDrawing(const string &buttonCondition)
{
	// Display the name of and distance to the selected system.
	DrawSelectedSystem();

	// Draw the buttons to switch to other map modes.

	// Remember which buttons we're showing.
	MapPanel::buttonCondition = buttonCondition;

	Information info;
	info.SetCondition(buttonCondition);
	const Interface *mapInterface = GameData::Interfaces().Get("map");
	if(player.MapZoom() >= static_cast<int>(mapInterface->GetValue("max zoom")))
		info.SetCondition("max zoom");
	if(player.MapZoom() <= static_cast<int>(mapInterface->GetValue("min zoom")))
		info.SetCondition("min zoom");
	const Interface *mapButtonUi = GameData::Interfaces().Get(Screen::Width() < 1300
		? "map buttons (small screen)" : "map buttons");
	mapButtonUi->Draw(info, this);

	// Draw the tooltips.
	if(hoverSystem && tooltip.ShouldDraw())
	{
		// Create the tooltip text.
		if(!tooltip.HasText())
		{
			MapPanel::SystemTooltipData t = escortSystems.at(hoverSystem);

			string text;
			if(hoverSystem == &playerSystem)
			{
				if(player.Flagship())
					--t.activeShips;
				if(t.activeShips || t.parkedShips || !t.outfits.empty())
					text = "You are here, with:\n";
				else
					text = "You are here.";
			}
			// If you have both active and parked escorts, call the active ones
			// "active escorts." Otherwise, just call them "escorts."
			if(t.activeShips && t.parkedShips)
				text += to_string(t.activeShips) + (t.activeShips == 1 ? " active escort\n" : " active escorts\n");
			else if(t.activeShips)
				text += to_string(t.activeShips) + (t.activeShips == 1 ? " escort" : " escorts");
			if(t.parkedShips)
				text += to_string(t.parkedShips) + (t.parkedShips == 1 ? " parked escort" : " parked escorts");
			if(!t.outfits.empty())
			{
				if(t.activeShips || t.parkedShips)
					text += "\n";

				unsigned sum = 0;
				for(const auto &it : t.outfits)
					sum += it.second;

				text += to_string(sum) + (sum == 1 ? " stored outfit" : " stored outfits");

				if(HasMultipleLandablePlanets(*hoverSystem) || t.outfits.size() > 1)
					for(const auto &it : t.outfits)
						text += "\n - " + to_string(it.second) + " on " + it.first->DisplayName();
			}

			tooltip.SetText(text);
		}
		tooltip.SetZone((hoverSystem->Position() + center) * Zoom(), Point(20., 20.));
		tooltip.Draw();
	}

	// Draw a warning if the selected system is not routable.
	if(selectedSystem != &playerSystem && !distance.HasRoute(*selectedSystem))
	{
		static const string NO_SHIP = "You do not have a flagship to jump with!";
		static const string NO_DRIVE = "You do not have a drive installed to be able to jump!";
		static const string UNAVAILABLE = "You have no available route to this system.";
		static const string UNKNOWN = "You have not yet mapped a route to this system.";
		const Ship *flagship = player.Flagship();
		if(!flagship)
			info.SetString("route error", NO_SHIP);
		else if(!flagship->JumpNavigation().HasAnyDrive())
			info.SetString("route error", NO_DRIVE);
		else if(player.CanView(*selectedSystem))
			info.SetString("route error", UNAVAILABLE);
		else
			info.SetString("route error", UNKNOWN);
	}

	mapInterface->Draw(info, this);
}



bool MapPanel::AllowsFastForward() const noexcept
{
	return true;
}



void MapPanel::UpdateTooltipActivation()
{
	tooltip.UpdateActivationCount();
}



bool MapPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	// When changing the map mode, explicitly close all child panels (for example, scrollable text boxes).
	auto removeChildren = [this]()
	{
		for(auto &child : GetChildren())
			RemoveChild(child.get());
	};
	if(command.Has(Command::MAP) || key == 'd' || key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == 's' && buttonCondition != "is shipyards")
	{
		GetUI()->Pop(this);
		removeChildren();
		GetUI()->Push(new MapShipyardPanel(*this));
	}
	else if(key == 'o' && buttonCondition != "is outfitters")
	{
		GetUI()->Pop(this);
		removeChildren();
		GetUI()->Push(new MapOutfitterPanel(*this));
	}
	else if(key == 'i' && buttonCondition != "is missions")
	{
		GetUI()->Pop(this);
		removeChildren();
		GetUI()->Push(new MissionPanel(*this));
	}
	else if(key == 'p' && buttonCondition != "is ports")
	{
		GetUI()->Pop(this);
		removeChildren();
		GetUI()->Push(new MapDetailPanel(*this, false));
	}
	else if(key == 't' && buttonCondition != "is stars")
	{
		GetUI()->Pop(this);
		removeChildren();
		GetUI()->Push(new MapDetailPanel(*this, true));
	}
	else if(key == 'f')
	{
		GetUI()->Push(new Dialog(
			this, &MapPanel::Find, "Search for:", "", Truncate::NONE, true));
		return true;
	}
	else if(key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS)
		IncrementZoom();
	else if(key == SDLK_MINUS || key == SDLK_KP_MINUS)
		DecrementZoom();
	else
		return false;

	UI::PlaySound(UI::UISound::SOFT);
	return true;
}



bool MapPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;

	// Figure out if a system was clicked on.
	Point click = Point(x, y) / Zoom() - center;
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		if(system.IsValid() && !system.Inaccessible() && click.Distance(system.Position()) < 10.
				&& (player.HasSeen(system) || &system == specialSystem))
		{
			Select(&system);
			break;
		}
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
		tooltip.Clear();
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
	if(dy > 0.)
		IncrementZoom();
	else if(dy < 0.)
		DecrementZoom();

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



Color MapPanel::DangerColor(const double danger)
{
	if(std::isnan(danger))
		return *GameData::Colors().Get("map danger none");
	else if(danger > .5)
		return Color(.6, .4 * (2. - 2. * min(1., danger)), 0., .4);
	else
		return MapColor(2. * danger - 1.);
}



Color MapPanel::UninhabitedColor()
{
	return GovernmentColor(GameData::Governments().Get("Uninhabited"));
}



Color MapPanel::UnexploredColor()
{
	return *GameData::Colors().Get("map system ring unexplored");
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
	// Update the cache to apply any visual changes needed after the selected system was changed.
	UpdateCache();

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
		const System *planEnd = plan.front();
		if(system == planEnd)
			return;

		RoutePlan addedRoute(*planEnd, *system, &player);
		if(!addedRoute.HasRoute())
			return;

		vector<const System *> newPlan = addedRoute.Plan();
		plan.insert(plan.begin(), newPlan.begin(), newPlan.end());
	}
	else if(distance.HasRoute(*system))
	{
		if(!isJumping)
			flagship->SetTargetSystem(nullptr);

		plan = distance.Plan(*system);
		if(isJumping)
			plan.push_back(source);
	}

	// Reset the travel destination if the final system in the travel plan has changed.
	if(!plan.empty())
		player.SetTravelDestination(nullptr);
}



void MapPanel::Find(const string &name)
{
	int bestIndex = 9999;
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		if(system.IsValid() && !system.Inaccessible() && player.CanView(system))
		{
			int index = Format::Search(it.first, name);
			if(index >= 0 && index < bestIndex)
			{
				bestIndex = index;
				selectedSystem = &system;
				UpdateCache();
				CenterOnSystem(selectedSystem);
				if(!index)
				{
					selectedPlanet = nullptr;
					return;
				}
			}
		}
	}
	for(const auto &it : GameData::Planets())
	{
		const Planet &planet = it.second;
		if(planet.IsValid() && player.CanView(*planet.GetSystem()))
		{
			int index = Format::Search(it.first, name);
			if(index >= 0 && index < bestIndex)
			{
				bestIndex = index;
				selectedSystem = planet.GetSystem();
				UpdateCache();
				CenterOnSystem(selectedSystem);
				if(!index)
				{
					selectedPlanet = &planet;
					return;
				}
			}
		}
	}
}



double MapPanel::Zoom() const
{
	return pow(1.5, zoom.AnimatedValue());
}



// Check whether the NPC and waypoint conditions of the given mission have
// been satisfied.
bool MapPanel::IsSatisfied(const Mission &mission) const
{
	return IsSatisfied(player, mission);
}



bool MapPanel::IsSatisfied(const PlayerInfo &player, const Mission &mission)
{
	return mission.IsSatisfied(player) && !mission.IsFailed();
}



bool MapPanel::GetTravelInfo(const System *previous, const System *next, const double jumpRange,
	bool &isJump, bool &isWormhole, bool &isMappable, Color *wormholeColor) const
{
	const bool isHyper = previous->Links().contains(next);
	isWormhole = false;
	isMappable = false;
	// Short-circuit the loop for MissionPanel, which draws hyperlinks and wormholes the same.
	if(!isHyper || wormholeColor)
		for(const StellarObject &object : previous->Objects())
			if(object.HasSprite() && object.HasValidPlanet()
				&& object.GetPlanet()->IsWormhole()
				&& player.HasVisited(*object.GetPlanet())
				&& player.CanView(*previous) && player.CanView(*next)
				&& &object.GetPlanet()->GetWormhole()->WormholeDestination(*previous) == next)
			{
				isWormhole = true;
				if(object.GetPlanet()->GetWormhole()->IsMappable())
				{
					isMappable = true;
					if(wormholeColor)
						*wormholeColor = *object.GetPlanet()->GetWormhole()->GetLinkColor();
					break;
				}
			}
	isJump = !isHyper && !isWormhole && previous->JumpNeighbors(jumpRange).contains(next);
	return isHyper || isWormhole || isJump;
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

	// Get danger level range so we can scale by it.
	double dangerMax = 0.;
	double dangerScale = 1.;
	if(commodity == SHOW_DANGER)
	{
		// Scale danger to span [0, 1] based on known systems, without including raid fleets
		// as those can greatly skew the range once they start having a chance of appearing,
		// leading to silly (and not very useful) displays.
		double dangerMin = numeric_limits<double>::max();
		for(const auto &it : GameData::Systems())
		{
			const System &system = it.second;

			// Only check displayed systems.
			if(!system.IsValid() || system.Inaccessible() || !player.HasVisited(system))
				continue;

			const double danger = DangerFleetTotal(player, system, false);
			if(danger > 0.)
			{
				if(dangerMax < danger)
					dangerMax = danger;
				if(dangerMin > danger)
					dangerMin = danger;
			}
		}
		if(dangerMax)
			dangerScale = 1. / log(dangerMin / dangerMax);
	}

	// Draw the circles for the systems, colored based on the selected criterion,
	// which may be government, services, or commodity prices.
	const Color &closeNameColor = *GameData::Colors().Get("map name");
	const Color &farNameColor = closeNameColor.Transparent(.5);
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		// Ignore systems which are inaccessible or have been referred to, but not actually defined.
		if(!system.IsValid() || system.Inaccessible())
			continue;
		// Ignore systems the player has never seen, unless they have a pending mission that lets them see it.
		if(!player.HasSeen(system) && &system != specialSystem)
			continue;

		Color color = UninhabitedColor();
		if(!player.CanView(system))
			color = UnexploredColor();
		else if(system.IsInhabited(player.Flagship()) || commodity == SHOW_SPECIAL
				|| commodity == SHOW_VISITED || commodity == SHOW_DANGER)
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
						if(object.HasSprite() && object.HasValidPlanet())
							size += object.GetPlanet()->ShipyardStock().size();
					value = size ? min(10., size) / 10. : -1.;
				}
				else if(commodity == SHOW_OUTFITTER)
				{
					double size = 0;
					for(const StellarObject &object : system.Objects())
						if(object.HasSprite() && object.HasValidPlanet())
							size += object.GetPlanet()->OutfitterStock().size();
					value = size ? min(60., size) / 60. : -1.;
				}
				else if(commodity == SHOW_VISITED)
				{
					bool all = true;
					bool some = false;
					colorSystem = false;
					for(const StellarObject &object : system.Objects())
						if(object.HasSprite() && object.HasValidPlanet() && !object.GetPlanet()->IsWormhole()
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
			else if(commodity == SHOW_DANGER)
			{
				const double danger = DangerFleetTotal(player, system, true);
				if(danger > 0.)
					color = DangerColor(1. - dangerScale * log(danger / dangerMax));
				else
					color = DangerColor(numeric_limits<double>::quiet_NaN());
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
					if(object.HasSprite() && object.HasValidPlanet())
					{
						const Planet *planet = object.GetPlanet();
						bool hasServices = planet->HasServices();
						hasSpaceport |= !planet->IsWormhole() && hasServices;
						if(planet->IsWormhole() || !planet->IsAccessible(player.Flagship()))
							continue;
						canLand |= planet->CanLand() && hasServices;
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

		static const vector<const Sprite *> unmappedSystem = {SpriteSet::Get("map/unexplored-star")};

		const bool canViewSystem = player.CanView(system);
		nodes.emplace_back(system.Position(), color,
			player.KnowsName(system) ? system.DisplayName() : "",
			(&system == &playerSystem || &system == selectedSystem) ? closeNameColor : farNameColor,
			canViewSystem ? system.GetGovernment() : nullptr,
			canViewSystem ? system.GetMapIcons() : unmappedSystem);
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
				// viewable. Also, avoid drawing twice by only drawing in the
				// direction of increasing pointer values.
				if((!player.CanView(*system) && !player.CanView(*link)) || !link->IsValid())
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

	// At each point in the path, keep track of how many ships in the
	// fleet are able to make it this far.
	const Ship *flagship = player.Flagship();
	if(!flagship)
		return;

	bool stranded = false;
	bool hasEscort = false;
	map<const Ship *, double> fuel;
	for(const shared_ptr<Ship> &it : player.Ships())
		if(!it->IsParked() && (!it->CanBeCarried() || it.get() == flagship) && it->GetSystem() == flagship->GetSystem())
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

	const double jumpRange = flagship->JumpNavigation().JumpRange();
	const System *previous = &playerSystem;
	const System *next = nullptr;
	for(int i = player.TravelPlan().size() - 1; i >= 0; --i, previous = next)
	{
		next = player.TravelPlan()[i];

		bool isJump, isWormhole, isMappable;
		Color wormholeColor;
		if(!GetTravelInfo(previous, next, jumpRange, isJump, isWormhole, isMappable, &wormholeColor))
			break;
		if(isWormhole && !isMappable)
			continue;

		// Wormholes cost nothing to go through. If this is not a wormhole,
		// check how much fuel every ship will expend to go through it.
		if(!isWormhole)
			for(auto &it : fuel)
				if(it.second >= 0.)
				{
					double cost = it.first->JumpNavigation().GetCheapestJumpType(previous, next).second;
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

		Point from = Zoom() * (previous->Position() + center);
		Point to = Zoom() * (next->Position() + center);
		const Point unit = (to - from).Unit();
		from += LINK_OFFSET * unit;
		to -= LINK_OFFSET * unit;

		// Non-hyperspace jumps are drawn with a dashed line.
		if(isJump)
			LineShader::DrawDashed(from, to, unit, 1.6f, drawColor, 11., 4.);
		else
			LineShader::Draw(from, to, 1.6f, drawColor);
	}
}



// Display the name of and distance to the selected system.
void MapPanel::DrawSelectedSystem()
{
	const Sprite *sprite = SpriteSet::Get("ui/selected system");
	SpriteShader::Draw(sprite, Point(0. + selectedSystemOffset, Screen::Top() + .5f * sprite->Height()));

	string text;
	if(!player.KnowsName(*selectedSystem))
		text = "Selected system: unexplored system";
	else
		text = "Selected system: " + selectedSystem->DisplayName();

	int jumps = 0;
	const vector<const System *> &plan = player.TravelPlan();
	auto it = find(plan.begin(), plan.end(), selectedSystem);
	if(it != plan.end())
		jumps = plan.end() - it;
	else if(distance.HasRoute(*selectedSystem))
		jumps = distance.Days(*selectedSystem);

	if(jumps == 1)
		text += " (1 jump away)";
	else if(jumps > 0)
		text += " (" + to_string(jumps) + " jumps away)";

	const Font &font = FontSet::Get(14);
	Point pos(-175. + selectedSystemOffset, Screen::Top() + .5 * (30. - font.Height()));
	font.Draw({text, {350, Alignment::CENTER, Truncate::MIDDLE}},
		pos, *GameData::Colors().Get("bright"));

	// Reset the position of this UI element. If something is in the way, it will be
	// moved back before it's drawn the next frame.
	selectedSystemOffset = 0;
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
			if(squad.second.activeShips || squad.second.parkedShips)
				RingShader::Draw(pos, INNER - 1.f, 0.f, squad.second.activeShips ? active : parked);

			if(!squad.second.outfits.empty())
				// Stored outfits are drawn/indicated by 8 short rays out of the system center.
				for(int i = 0; i < 8; ++i)
				{
					static constexpr float WIDTH = 1.6f;

					// Starting at 7.5 degrees to intentionally mis-align with mission pointers.
					Angle angle = Angle(7.5f + 45.f * i);
					// Account for how rounded caps extend out by an additional WIDTH.
					Point from = pos + angle.Unit() * (OUTER + WIDTH);
					Point to = from + angle.Unit() * (4.f - WIDTH);
					LineShader::Draw(from, to, WIDTH, active);
				}
		}
}



void MapPanel::DrawWormholes()
{
	// Keep track of what arrows and links need to be drawn.
	vector<WormholeArrow> arrowsToDraw;

	// A system can host more than one set of wormholes (e.g. Cardea), and some wormholes may even
	// share a link vector.
	for(auto &&it : GameData::Wormholes())
	{
		if(!it.second.IsValid())
			continue;

		const Planet &p = *it.second.GetPlanet();
		if(!p.IsValid() || !player.HasVisited(p) || !it.second.IsMappable())
			continue;

		for(auto &&link : it.second.Links())
			if(!link.first->Inaccessible() && !link.second->Inaccessible() && p.IsInSystem(link.first)
					&& player.CanView(*link.first) && player.CanView(*link.second))
				arrowsToDraw.emplace_back(link.first, link.second, it.second.GetLinkColor());

	}

	static const double ARROW_LENGTH = 4.;
	static const double ARROW_RATIO = .3;
	static const Angle LEFT(30.);
	static const Angle RIGHT(-30.);
	const double zoom = Zoom();

	for(const WormholeArrow &link : arrowsToDraw)
	{
		// Get the wormhole link color.
		const Color &arrowColor = *link.color;
		const Color &wormholeDim = Color::Multiply(.33f, arrowColor);

		// Compute the start and end positions of the wormhole link.
		Point from = zoom * (link.from->Position() + center);
		Point to = zoom * (link.to->Position() + center);
		Point offset = (from - to).Unit() * LINK_OFFSET;
		from -= offset;
		to += offset;

		// If an arrow is being drawn, the link will always be drawn too. Draw
		// the link only for the first instance of it in this set.
		if(link.from < link.to || !count_if(arrowsToDraw.begin(), arrowsToDraw.end(),
			[&link](const WormholeArrow &cmp)
			{ return cmp.from == link.to && cmp.to == link.from; }))
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
	BatchDrawList starBatch;
	double zoom = Zoom();
	for(const Node &node : nodes)
	{
		Point pos = zoom * (node.position + center);
		if(commodity != SHOW_STARS)
			RingShader::Draw(pos, OUTER, INNER, node.color);
		else
		{
			// Ensures every multiple-star system has a characteristic, deterministic rotation.
			Angle starAngle = 0;
			Angle angularSpacing = 0;
			Point starOffset = Point(0, 0);

			const int starsToDraw = min<int>(node.mapIcons.size(), MAX_STARS);
			if(starsToDraw > 1)
			{
				starAngle = node.name.length() + node.position.Length();
				angularSpacing = 360. / starsToDraw;
				starOffset = starsToDraw * Point(2., 2.);
			}

			// Draw the star icons.
			for(int i = 0; i < starsToDraw; ++i)
			{
				starAngle += angularSpacing;
				const Sprite *star = node.mapIcons[i];
				const Body starBody(star, pos + zoom * starOffset * starAngle.Unit(),
					Point(0, 0), 0, cbrt(zoom) * 0.6, Point(1., 1.), 0.8);
				starBatch.Add(starBody);
			}
		}

		if(commodity == SHOW_GOVERNMENT && node.government && node.government->DisplayName() != "Uninhabited")
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
	starBatch.Draw();
	starBatch.Clear();
}



void MapPanel::DrawNames()
{
	// Draw names for all systems you have visited.
	double zoom = Zoom();
	if(zoom <= .5)
		return;
	static constexpr double BIGGER_TARGET_ZOOM = 2. / 3.;
	double alpha = zoom >= BIGGER_TARGET_ZOOM ? 1. : (.5 - zoom) / (.5 - BIGGER_TARGET_ZOOM);
	bool useBigFont = (zoom > 2.);
	const Font &font = FontSet::Get(useBigFont ? 18 : 14);
	Point offset(useBigFont ? 8. : 6., -.5 * font.Height());
	for(const Node &node : nodes)
		font.Draw(node.name, zoom * (node.position + center) + offset, node.nameColor.Transparent(alpha));
}



void MapPanel::DrawMissions()
{
	// Draw a pointer for each active or available mission.
	map<const System *, PointerDrawCount> missionCount;

	const Set<Color> &colors = GameData::Colors();
	const Color &availableColor = *colors.Get("available job");
	const Color &unavailableColor = *colors.Get("unavailable job");
	const Color &currentColor = *colors.Get("active mission");
	const Color &blockedColor = *colors.Get("blocked mission");
	const Color &specialColor = *colors.Get("special mission");
	const Color &waypointColor = *colors.Get("waypoint");
	if(specialSystem)
	{
		// The special system pointer is larger than the others.
		++missionCount[specialSystem].drawn;
		Angle a = Angle(MISSION_POINTERS_ANGLE_DELTA * missionCount[specialSystem].drawn);
		Point pos = Zoom() * (specialSystem->Position() + center);
		PointerShader::Draw(pos, a.Unit(), 20.f, 27.f, -4.f, black);
		PointerShader::Draw(pos, a.Unit(), 11.5f, 21.5f, -6.f, specialColor);
	}
	// Calculate the available (and unavailable) jobs, but don't draw them yet.
	for(const Mission &mission : player.AvailableJobs())
	{
		const System *system = mission.Destination()->GetSystem();
		if(!system)
			continue;
		auto &it = missionCount[system];
		if(mission.CanAccept(player))
			++it.available;
		else
			++it.unavailable;
	}
	for_each(missionCount.begin(), missionCount.end(), [](auto &it) { it.second.Reserve(); });
	for(const Mission &mission : player.Missions())
	{
		if(!mission.IsVisible())
			continue;

		const System *system = mission.Destination()->GetSystem();
		if(!system)
			continue;

		auto &it = missionCount[system];
		if(it.drawn < it.MaximumActive())
		{
			pair<bool, bool> blink = BlinkMissionIndicator(player, mission, step);
			bool isSatisfied = IsSatisfied(player, mission) && blink.second;
			const Color &color = blink.first ? black : isSatisfied ? currentColor : blockedColor;
			DrawPointer(system, it.drawn, it.MaximumActive(), color, isSatisfied);
		}

		for(const System *waypoint : mission.Waypoints())
			DrawPointer(waypoint, missionCount[waypoint].drawn, missionCount[waypoint].MaximumActive(), waypointColor);
		for(const Planet *stopover : mission.Stopovers())
		{
			const System *stopoverSystem = stopover->GetSystem();
			auto &counts = missionCount[stopoverSystem];
			DrawPointer(stopoverSystem, counts.drawn, counts.MaximumActive(), waypointColor);
		}
		for(const System *mark : mission.MarkedSystems())
			DrawPointer(mark, missionCount[mark].drawn, missionCount[mark].MaximumActive(), waypointColor);
		for(const System *mark : mission.TrackedSystems())
			DrawPointer(mark, missionCount[mark].drawn, missionCount[mark].MaximumActive(), waypointColor);
	}
	// Draw the available and unavailable jobs.
	for(auto &&it : missionCount)
	{
		const auto &system = it.first;
		auto &&counters = it.second;
		for(unsigned i = 0; i < counters.available; ++i)
			DrawPointer(system, counters.drawn, MAX_MISSION_POINTERS_DRAWN, availableColor);
		for(unsigned i = 0; i < counters.unavailable; ++i)
			DrawPointer(system, counters.drawn, MAX_MISSION_POINTERS_DRAWN, unavailableColor);
	}
}



void MapPanel::DrawPointer(const System *system, unsigned &systemCount, unsigned max, const Color &color, bool bigger)
{
	if(systemCount >= max)
		return;
	DrawPointer(Zoom() * (system->Position() + center), systemCount, color, true, bigger);
}



void MapPanel::IncrementZoom()
{
	const Interface *mapInterface = GameData::Interfaces().Get("map");
	double newZoom = min<double>(mapInterface->GetValue("max zoom"), player.MapZoom() + 1);
	zoom.Set(newZoom, mapInterface->GetValue("zoom animation duration"));
	player.SetMapZoom(newZoom);
}


void MapPanel::DecrementZoom()
{
	const Interface *mapInterface = GameData::Interfaces().Get("map");
	double newZoom = max<double>(mapInterface->GetValue("min zoom"), player.MapZoom() - 1);
	zoom.Set(newZoom, mapInterface->GetValue("zoom animation duration"));
	player.SetMapZoom(newZoom);
}
