/* MissionPanel.cpp
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



#include "MissionPanel.h"

#include "text/Alignment.h"
#include "audio/Audio.h"
#include "Command.h"
#include "CoreStartData.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "shader/LineShader.h"
#include "Mission.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "shader/RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "System.h"
#include "TextArea.h"
#include "text/Truncate.h"
#include "UI.h"

#include <algorithm>
#include <cmath>
#include <sstream>

using namespace std;

namespace {
	constexpr int SIDE_WIDTH = 280;

	// Check if the mission involves the given system,
	bool Involves(const Mission &mission, const System *system)
	{
		if(!system)
			return false;

		if(mission.Destination()->IsInSystem(system))
			return true;

		for(const System *waypoint : mission.Waypoints())
			if(waypoint == system)
				return true;

		for(const Planet *stopover : mission.Stopovers())
			if(stopover->IsInSystem(system))
				return true;

		for(const System *mark : mission.MarkedSystems())
			if(mark == system)
				return true;

		return false;
	}

	size_t MaxDisplayedMissions(bool onRight)
	{
		return static_cast<unsigned>(max(0, static_cast<int>(floor((Screen::Height() - (onRight ? 160. : 190.)) / 20.))));
	}

	// Compute the required scroll amount for the given list of jobs/missions.
	void ScrollMissionList(const list<Mission> &missionList, const list<Mission>::const_iterator &it,
		double &sideScroll, bool checkVisibility)
	{
		// We don't need to scroll at all if the selection must be within the viewport. The current
		// scroll could be non-zero if missions were added/aborted, so return the delta that will reset it.
		const auto maxViewable = MaxDisplayedMissions(checkVisibility);
		const auto missionCount = missionList.size();
		if(missionCount < maxViewable)
			sideScroll = 0;
		else
		{
			const auto countBefore = static_cast<size_t>(checkVisibility
					? count_if(missionList.begin(), it, [](const Mission &m) { return m.IsVisible(); })
					: distance(missionList.begin(), it));

			const auto maximumScroll = (missionCount - maxViewable) * 20.;
			const auto pageScroll = maxViewable * 20.;
			const auto desiredScroll = countBefore * 20.;
			const auto bottomOfPage = sideScroll + pageScroll;
			if(desiredScroll < sideScroll)
			{
				// Scroll upwards.
				sideScroll = desiredScroll;
			}
			else if(desiredScroll > bottomOfPage)
			{
				// Scroll downwards (but not so far that the list's bottom sprite comes upwards further than needed).
				sideScroll = min(maximumScroll, sideScroll + (desiredScroll - bottomOfPage));
			}
		}
	}
}



// Open the missions panel directly.
MissionPanel::MissionPanel(PlayerInfo &player)
	: MapPanel(player),
	missionInterface(GameData::Interfaces().Get("mission")),
	available(player.AvailableJobs()),
	accepted(player.Missions()),
	availableIt(player.AvailableJobs().begin()),
	acceptedIt(player.AvailableJobs().empty() ? accepted.begin() : accepted.end()),
	tooltip(150, Alignment::LEFT, Tooltip::Direction::DOWN_RIGHT, Tooltip::Corner::BOTTOM_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium"))
{
	// Re-do job sorting since something could have changed
	player.SortAvailable();

	while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
		++acceptedIt;

	InitTextArea();

	// Select the first available or accepted mission in the currently selected
	// system, or along the travel plan.
	if(!FindMissionForSystem(selectedSystem) && player.HasTravelPlan())
	{
		const auto &tp = player.TravelPlan();
		for(auto it = tp.crbegin(); it != tp.crend(); ++it)
			if(FindMissionForSystem(*it))
				break;
	}

	SetSelectedScrollAndCenter(true);
}



// Switch to the missions panel from another map panel.
MissionPanel::MissionPanel(const MapPanel &panel)
	: MapPanel(panel),
	missionInterface(GameData::Interfaces().Get("mission")),
	available(player.AvailableJobs()),
	accepted(player.Missions()),
	availableIt(player.AvailableJobs().begin()),
	acceptedIt(player.AvailableJobs().empty() ? accepted.begin() : accepted.end()),
	availableScroll(0), acceptedScroll(0), dragSide(0),
	tooltip(150, Alignment::LEFT, Tooltip::Direction::DOWN_RIGHT, Tooltip::Corner::BOTTOM_LEFT,
		GameData::Colors().Get("tooltip background"), GameData::Colors().Get("medium"))
{
	Audio::Pause();

	// Re-do job sorting since something could have changed
	player.SortAvailable();

	// In this view, always color systems based on player reputation.
	commodity = SHOW_REPUTATION;

	while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
		++acceptedIt;

	InitTextArea();

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

	SetSelectedScrollAndCenter();
}



void MissionPanel::Step()
{
	MapPanel::Step();

	// If a job or mission that launches the player triggers,
	// immediately close the map.
	if(player.ShouldLaunch())
		GetUI()->Pop(this);

	if(GetUI()->IsTop(this) && player.GetPlanet() && player.GetDate() >= player.StartData().GetDate() + 12)
		DoHelp("map advanced");
	DoHelp("jobs");

	if(GetUI()->IsTop(this) && !fromMission)
	{
		Mission *mission = player.MissionToOffer(Mission::JOB_BOARD);
		if(mission)
		{
			// Prevent dragging while a mission is offering, to make sure
			// the screen fully pans to the destination system.
			canDrag = false;

			// Only highlight and pan to the destination if the mission is visible.
			// Otherwise, pan to the player's current system.
			specialSystem = mission->IsVisible() ? mission->Destination()->GetSystem() : nullptr;
			CenterOnSystem(specialSystem ? specialSystem : player.GetSystem());

			mission->Do(Mission::OFFER, player, GetUI());
		}
		else
		{
			canDrag = true;
			specialSystem = nullptr;
			player.HandleBlockedMissions(Mission::JOB_BOARD, GetUI());
		}
	}
}



void MissionPanel::Draw()
{
	MapPanel::Draw();

	// Update the tooltip timer.
	if(hoverSort >= 0)
		tooltip.IncrementCount();
	else
		tooltip.DecrementCount();

	const Set<Color> &colors = GameData::Colors();

	const Color &routeColor = *colors.Get("mission route");
	const Ship *flagship = player.Flagship();
	const double jumpRange = flagship ? flagship->JumpNavigation().JumpRange() : 0.;
	const System *previous = player.GetSystem();
	const vector<const System *> plan = distance.Plan(*selectedSystem);
	for(auto it = plan.rbegin(); it != plan.rend(); ++it)
	{
		const System *next = *it;

		bool isJump, isWormhole, isMappable;
		if(!GetTravelInfo(previous, next, jumpRange, isJump, isWormhole, isMappable, nullptr))
			break;
		if(isWormhole && !isMappable)
			continue;

		Point from = Zoom() * (previous->Position() + center);
		Point to = Zoom() * (next->Position() + center);
		const Point unit = (to - from).Unit();
		from += LINK_OFFSET * unit;
		to -= LINK_OFFSET * unit;

		// Non-hyperspace jumps are drawn with a dashed line.
		if(isJump)
			LineShader::DrawDashed(from, to, unit, 3.f, routeColor, 11., 4.);
		else
			LineShader::Draw(from, to, 3.f, routeColor);

		previous = next;
	}

	const Color &availableColor = *colors.Get("available back");
	const Color &unavailableColor = *colors.Get("unavailable back");
	const Color &currentColor = *colors.Get("active back");
	const Color &blockedColor = *colors.Get("blocked back");
	if(availableIt != available.end() && availableIt->Destination())
		DrawMissionSystem(*availableIt, CanAccept() ? availableColor : unavailableColor);
	if(acceptedIt != accepted.end() && acceptedIt->Destination())
		DrawMissionSystem(*acceptedIt, IsSatisfied(*acceptedIt) ? currentColor : blockedColor);

	Point pos;
	if(player.GetPlanet())
	{
		pos = DrawPanel(
			Screen::TopLeft() + Point(0., -availableScroll),
			"Missions available here:",
			available.size(),
			true);
		DrawList(available, pos, availableIt, true);
	}

	pos = DrawPanel(
		Screen::TopRight() + Point(-SIDE_WIDTH, -acceptedScroll),
		"Your current missions:",
		AcceptedVisible());
	DrawList(accepted, pos, acceptedIt);

	// Now that the mission lists and map elements are drawn, draw the top-most UI elements.
	DrawKey();
	DrawMissionInfo();
	DrawTooltips();
	FinishDrawing("is missions");
}



void MissionPanel::UpdateTooltipActivation()
{
	MapPanel::UpdateTooltipActivation();
	tooltip.UpdateActivationCount();
}



// Only override the ones you need; the default action is to return false.
bool MissionPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	UI::UISound sound = UI::UISound::NORMAL;
	if(command.Has(Command::HELP))
	{
		sound = UI::UISound::NONE;
		DoHelp("jobs", true);
		DoHelp("map advanced", true);
	}
	else if(key == 'a' && CanAccept())
	{
		Accept((mod & KMOD_CTRL));
		return true;
	}
	else if(key == 'A' || (key == 'a' && (mod & KMOD_SHIFT)))
	{
		if(acceptedIt != accepted.end() && acceptedIt->IsVisible())
			GetUI()->Push(new Dialog(this, &MissionPanel::AbortMission,
				"Abort mission \"" + acceptedIt->DisplayName() + "\"?"));
		return true;
	}
	else if(key == SDLK_LEFT && availableIt == available.end())
	{
		// Switch to the first mission in the "available missions" list.
		acceptedIt = accepted.end();
		availableIt = available.begin();
	}
	else if(key == SDLK_RIGHT && acceptedIt == accepted.end() && AcceptedVisible())
	{
		// Switch to the first mission in the "accepted missions" list.
		availableIt = available.end();
		acceptedIt = accepted.begin();
		while(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
			++acceptedIt;
	}
	else if(key == SDLK_UP)
	{
		SelectAnyMission();
		// Select the previous mission, which may be at the end of the list.
		if(availableIt != available.end())
		{
			// All available missions are, by definition, visible.
			if(availableIt == available.begin())
				availableIt = available.end();
			--availableIt;
		}
		else if(acceptedIt != accepted.end())
		{
			// Skip over any invisible, accepted missions.
			do {
				if(acceptedIt == accepted.begin())
					acceptedIt = accepted.end();
				--acceptedIt;
			} while(!acceptedIt->IsVisible());
		}
	}
	else if(key == SDLK_DOWN)
	{
		if(SelectAnyMission())
		{
			// A mission was just auto-selected. Nothing else to do here.
		}
		else if(availableIt != available.end())
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
		return MapPanel::KeyDown(key, mod, command, isNewPress);

	// To reach here, we changed the selected mission. Scroll the active
	// mission list, update the selected system, and pan the map.
	SetSelectedScrollAndCenter();

	UI::PlaySound(sound);
	return true;
}



bool MissionPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return MapPanel::Click(x, y, button, clicks);

	dragSide = 0;

	if(x > Screen::Right() - 80 && y > Screen::Bottom() - 50)
		return DoKey('p');

	if(x < Screen::Left() + SIDE_WIDTH)
	{
		// Panel header
		if(y + static_cast<int>(availableScroll) < Screen::Top() + 30)
		{
			dragSide = -1;
			if(y + static_cast<int>(availableScroll) < Screen::Top() + 10)
			{
				// empty space
				return false;
			}
			// Sorter buttons
			else if(hoverSort >= 0)
			{
				if(hoverSort == 0)
					player.ToggleSortSeparateDeadline();
				else if(hoverSort == 1)
					player.ToggleSortSeparatePossible();
				else if(hoverSort == 2)
				{
					player.NextAvailableSortType();
					tooltip.Clear();
				}
				else if(hoverSort == 3)
					player.ToggleSortAscending();
				UI::PlaySound(UI::UISound::NORMAL);
				return true;
			}
			return false;
		}
		// Available missions
		unsigned index = max(0, (y + static_cast<int>(availableScroll) - 36 - Screen::Top()) / 20);
		if(index < available.size())
		{
			const auto lastAvailableIt = availableIt;
			availableIt = available.begin();
			while(index--)
				++availableIt;
			acceptedIt = accepted.end();
			dragSide = -1;
			if(availableIt == lastAvailableIt)
			{
				CycleInvolvedSystems(*availableIt);
				return true;
			}
			SetSelectedScrollAndCenter();
			return true;
		}
	}
	else if(x >= Screen::Right() - SIDE_WIDTH)
	{
		// Accepted missions
		int index = max(0, (y + static_cast<int>(acceptedScroll) - 36 - Screen::Top()) / 20);
		if(index < AcceptedVisible())
		{
			const auto lastAcceptedIt = acceptedIt;
			acceptedIt = accepted.begin();
			while(index || !acceptedIt->IsVisible())
			{
				index -= acceptedIt->IsVisible();
				++acceptedIt;
			}
			availableIt = available.end();
			dragSide = 1;
			if(lastAcceptedIt == acceptedIt)
			{
				CycleInvolvedSystems(*acceptedIt);
				return true;
			}
			SetSelectedScrollAndCenter();
			return true;
		}
	}

	// Figure out if a system was clicked on.
	Point click = Point(x, y) / Zoom() - center;
	const System *system = nullptr;
	for(const auto &it : GameData::Systems())
		if(it.second.IsValid() && click.Distance(it.second.Position()) < 10.
				&& (player.HasSeen(it.second) || &it.second == specialSystem))
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

		// When clicking a new system, select the first available mission
		// (instead of continuing from wherever the iterator happens to be)
		if((availableIt != available.end() && !Involves(*availableIt, system)) ||
			(acceptedIt != accepted.end() && !Involves(*acceptedIt, system)))
		{
			auto firstExistingIt = find_if(available.begin(), available.end(),
				[&system](const Mission &m) { return Involves(m, system); });

			if(firstExistingIt != available.end())
			{
				availableIt = firstExistingIt;
				acceptedIt = accepted.end();
			}
			else
			{
				firstExistingIt = find_if(accepted.begin(), accepted.end(),
					[&system](const Mission &m) { return m.IsVisible() && Involves(m, system); });

				if(firstExistingIt != accepted.end())
				{
					availableIt = available.end();
					acceptedIt = firstExistingIt;
				}
			}
		}
		else while(options--)
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

			if(availableIt != available.end() && Involves(*availableIt, system))
				break;
			if(acceptedIt != accepted.end() && Involves(*acceptedIt, system))
				break;
		}
		// Make sure invisible missions are never selected, even if there were
		// no other missions in this system.
		if(acceptedIt != accepted.end() && !acceptedIt->IsVisible())
			acceptedIt = accepted.end();
		// Scroll the relevant panel so that the mission highlighted is visible.
		if(availableIt != available.end())
			ScrollMissionList(available, availableIt, availableScroll, false);
		else if(acceptedIt != accepted.end())
			ScrollMissionList(accepted, acceptedIt, acceptedScroll, true);
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
	else if(canDrag)
		MapPanel::Drag(dx, dy);

	return true;
}



// Check to see if the mouse is over either of the mission lists.
bool MissionPanel::Hover(int x, int y)
{
	dragSide = 0;
	int oldSort = hoverSort;
	hoverSort = -1;
	unsigned index = max(0, (y + static_cast<int>(availableScroll) - 36 - Screen::Top()) / 20);
	if(x < Screen::Left() + SIDE_WIDTH)
	{
		if(index < available.size())
		{
			dragSide = -1;

			// Hovering over sort buttons
			if(y + static_cast<int>(availableScroll) < Screen::Top() + 30 && y >= Screen::Top() + 10
				&& x >= Screen::Left() + SIDE_WIDTH - 110)
			{
				hoverSort = (x - Screen::Left() - SIDE_WIDTH + 110) / 30;
				if(hoverSort > 3)
					hoverSort = -1;
			}
		}
	}
	else if(x >= Screen::Right() - SIDE_WIDTH)
	{
		if(static_cast<int>(index) < AcceptedVisible())
			dragSide = 1;
	}

	if(oldSort != hoverSort)
		tooltip.Clear();

	return dragSide || MapPanel::Hover(x, y);
}



bool MissionPanel::Scroll(double dx, double dy)
{
	if(dragSide)
		return Drag(0., dy * Preferences::ScrollSpeed());

	return MapPanel::Scroll(dx, dy);
}



void MissionPanel::Resize()
{
	ResizeTextArea();
}



void MissionPanel::InitTextArea()
{
	description = make_shared<TextArea>();
	description->SetFont(FontSet::Get(14));
	description->SetAlignment(Alignment::JUSTIFIED);
	description->SetColor(*GameData::Colors().Get("bright"));
	ResizeTextArea();
}



void MissionPanel::ResizeTextArea() const
{
	description->SetRect(missionInterface->GetBox("description"));
}



void MissionPanel::SetSelectedScrollAndCenter(bool immediate)
{
	// Auto select the destination system for the current mission.
	if(availableIt != available.end())
	{
		selectedSystem = availableIt->Destination()->GetSystem();
		UpdateCache();
		ScrollMissionList(available, availableIt, availableScroll, false);
	}
	else if(acceptedIt != accepted.end())
	{
		selectedSystem = acceptedIt->Destination()->GetSystem();
		UpdateCache();
		ScrollMissionList(accepted, acceptedIt, acceptedScroll, true);
	}

	// Center on the selected system.
	CenterOnSystem(selectedSystem, immediate);

	cycleInvolvedIndex = 0;
}



void MissionPanel::DrawKey() const
{
	const Sprite *back = SpriteSet::Get("ui/mission key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));

	const Font &font = FontSet::Get(14);
	Point angle = Point(1., 1.).Unit();

	const int ROWS = 5;
	Point pos(Screen::Left() + 10., Screen::Bottom() - ROWS * 20. + 5.);
	Point pointerOff(5., 5.);
	Point textOff(8., -.5 * font.Height());

	const Set<Color> &colors = GameData::Colors();
	const Color &bright = *colors.Get("bright");
	const Color &dim = *colors.Get("dim");
	const Color COLOR[ROWS] = {
		*colors.Get("available job"),
		*colors.Get("unavailable job"),
		*colors.Get("active mission"),
		*colors.Get("blocked mission"),
		*colors.Get("waypoint")
	};
	static const string LABEL[ROWS] = {
		"Available job; can accept",
		"Too little space to accept",
		"Active job; go here to complete",
		"Has unfinished requirements",
		"System of importance"
	};
	int selected = -1;
	if(availableIt != available.end())
		selected = 0 + !CanAccept();
	if(acceptedIt != accepted.end() && acceptedIt->Destination())
		selected = 2 + !IsSatisfied(*acceptedIt);

	for(int i = 0; i < ROWS; ++i)
	{
		PointerShader::Draw(pos + pointerOff, angle, 10.f, 18.f, 0.f, COLOR[i]);
		font.Draw(LABEL[i], pos + textOff, i == selected ? bright : dim);
		pos.Y() += 20.;
	}
}



// Highlight the systems associated with the given mission (i.e. destination and
// waypoints) by drawing colored rings around them.
void MissionPanel::DrawMissionSystem(const Mission &mission, const Color &color) const
{
	auto toVisit = set<const System *>{mission.Waypoints()};
	toVisit.insert(mission.MarkedSystems().begin(), mission.MarkedSystems().end());
	toVisit.insert(mission.TrackedSystems().begin(), mission.TrackedSystems().end());
	for(const Planet *planet : mission.Stopovers())
		toVisit.insert(planet->GetSystem());
	auto hasVisited = set<const System *>{mission.VisitedWaypoints()};
	hasVisited.insert(mission.UnmarkedSystems().begin(), mission.UnmarkedSystems().end());
	for(const Planet *planet : mission.VisitedStopovers())
		hasVisited.insert(planet->GetSystem());

	const Color &waypoint = *GameData::Colors().Get("waypoint back");
	const Color &visited = *GameData::Colors().Get("faint");

	double zoom = Zoom();
	auto drawRing = [&](const System *system, const Color &drawColor)
		{ RingShader::Add(zoom * (system->Position() + center), 22.f, 20.5f, drawColor); };

	RingShader::Bind();
	{
		// Draw a colored ring around the destination system.
		drawRing(mission.Destination()->GetSystem(), color);
		// Draw bright rings around systems that still need to be visited.
		for(const System *system : toVisit)
			drawRing(system, waypoint);
		// Draw faint rings around systems already visited for this mission.
		for(const System *system : hasVisited)
			drawRing(system, visited);
	}
	RingShader::Unbind();
}



// Draw the background for the lists of available and accepted missions (based on pos).
Point MissionPanel::DrawPanel(Point pos, const string &label, int entries, bool sorter) const
{
	const Color &back = *GameData::Colors().Get("map side panel background");
	const Color &text = *GameData::Colors().Get("medium");
	const Color separatorLine = text.Opaque();
	const Color &title = *GameData::Colors().Get("bright");
	const Color &highlight = *GameData::Colors().Get("dim");

	// Draw the panel.
	Point size(SIDE_WIDTH, 20 * entries + 40);
	FillShader::Fill(Rectangle::FromCorner(pos, size), back);

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

	const Font &font = FontSet::Get(14);
	pos += Point(10., 10. + (20. - font.Height()) * .5);

	// Panel sorting
	const Sprite *rush[2] = {
			SpriteSet::Get("ui/sort rush include"), SpriteSet::Get("ui/sort rush separate") };
	const Sprite *acceptable[2] = {
			SpriteSet::Get("ui/sort unacceptable include"), SpriteSet::Get("ui/sort unacceptable separate") };
	const Sprite *sortIcon[4] = {
			SpriteSet::Get("ui/sort abc"), SpriteSet::Get("ui/sort pay"),
			SpriteSet::Get("ui/sort speed"), SpriteSet::Get("ui/sort convenient") };
	const Sprite *arrow[2] = {
			SpriteSet::Get("ui/sort descending"), SpriteSet::Get("ui/sort ascending") };

	// Draw Sorting Columns
	if(entries && sorter)
	{
		SpriteShader::Draw(arrow[player.ShouldSortAscending()], pos + Point(SIDE_WIDTH - 15., 7.5));

		SpriteShader::Draw(sortIcon[player.GetAvailableSortType()], pos + Point(SIDE_WIDTH - 45., 7.5));

		SpriteShader::Draw(acceptable[player.ShouldSortSeparatePossible()], pos + Point(SIDE_WIDTH - 75., 7.5));

		SpriteShader::Draw(rush[player.ShouldSortSeparateDeadline()], pos + Point(SIDE_WIDTH - 105., 7.5));

		if(hoverSort >= 0)
		{
			Rectangle zone = Rectangle(pos + Point(SIDE_WIDTH - 105. + 30 * hoverSort, 7.5), Point(22., 16.));
			tooltip.SetZone(zone);
			FillShader::Fill(zone, highlight);
		}
	}

	// Panel title
	font.Draw(label, pos, title);
	FillShader::Fill(
		pos + Point(.5 * size.X() - 5., 15.),
		Point(size.X() - 10., 1.),
		separatorLine);

	pos.Y() += 5.;

	return pos;
}



Point MissionPanel::DrawList(const list<Mission> &missionList, Point pos, const list<Mission>::const_iterator &selectIt,
	bool separateDeadlineOrPossible) const
{
	const Font &font = FontSet::Get(14);
	const Color &highlight = *GameData::Colors().Get("faint");
	const Color &unselected = *GameData::Colors().Get("medium");
	const Color &selected = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("dim");
	const Sprite *fast = SpriteSet::Get("ui/fast forward");
	bool separated = false;

	for(auto it = missionList.begin(); it != missionList.end(); ++it)
	{
		if(!it->IsVisible())
			continue;

		pos.Y() += 20.;
		if(separateDeadlineOrPossible && !separated
				&& ((player.ShouldSortSeparateDeadline() && it->Deadline())
						|| (player.ShouldSortSeparatePossible() && !it->CanAccept(player))))
		{
			pos.Y() += 8.;
			separated = true;
		}

		bool isSelected = it == selectIt;
		if(isSelected)
			FillShader::Fill(
				pos + Point(.5 * SIDE_WIDTH - 5., 8.),
				Point(SIDE_WIDTH - 10., 20.),
				highlight);

		if(it->Deadline())
			SpriteShader::Draw(fast, pos + Point(-4., 8.));

		const Color *color = nullptr;
		bool canAccept = (&missionList == &available ? it->CanAccept(player) : IsSatisfied(*it));
		if(!canAccept)
		{
			if(it->Unavailable().IsLoaded())
				color = &it->Unavailable();
			else
				color = &dim;
		}
		else if(isSelected)
		{
			if(it->Selected().IsLoaded())
				color = &it->Selected();
			else
				color = &selected;
		}
		else
		{
			if(it->Unselected().IsLoaded())
				color = &it->Unselected();
			else
				color = &unselected;
		}

		font.Draw({it->DisplayName(), {SIDE_WIDTH - 11, Truncate::BACK}}, pos, *color);
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

	if(availableIt != available.end() || acceptedIt != accepted.end())
		info.SetCondition("has description");

	info.SetString("cargo free", Format::SimplePluralization(player.Cargo().Free(), "ton"));
	info.SetString("bunks free", Format::SimplePluralization(player.Cargo().BunksFree(), "bunk"));

	info.SetString("today", player.GetDate().ToString());

	missionInterface->Draw(info, this);

	// If a mission is selected, draw its descriptive text.
	if(availableIt != available.end())
	{
		if(!descriptionVisible)
		{
			AddChild(description);
			descriptionVisible = true;
		}
		description->SetText(availableIt->Description());
	}
	else if(acceptedIt != accepted.end())
	{
		if(!descriptionVisible)
		{
			AddChild(description);
			descriptionVisible = true;
		}
		description->SetText(acceptedIt->Description());
	}
	else if(descriptionVisible)
	{
		RemoveChild(description.get());
		descriptionVisible = false;
	}
}



void MissionPanel::DrawTooltips()
{
	if(hoverSort < 0 || !tooltip.ShouldDraw())
		return;

	// Create the tooltip text.
	if(!tooltip.HasText())
	{
		string text;
		if(hoverSort == 0)
			text = "Filter out missions with a deadline";
		else if(hoverSort == 1)
			text = "Filter out missions that you can't accept";
		else if(hoverSort == 2)
		{
			switch(player.GetAvailableSortType())
			{
				case 0:
					text = "Sort alphabetically";
					break;
				case 1:
					text = "Sort by payment";
					break;
				case 2:
					text = "Sort by distance";
					break;
				case 3:
					text = "Sort by convenience: "
							"Prioritize missions going to a planet or system that is already a destination of one of your missions";
					break;
			}
		}
		else if(hoverSort == 3)
			text = "Sort direction";
		tooltip.SetText(text);
	}

	tooltip.Draw();
}



bool MissionPanel::CanAccept() const
{
	if(availableIt == available.end())
		return false;

	return availableIt->CanAccept(player);
}



void MissionPanel::Accept(bool force)
{
	const Mission &toAccept = *availableIt;
	int cargoToSell = 0;
	if(toAccept.CargoSize())
		cargoToSell = toAccept.CargoSize() - player.Cargo().Free();
	int crewToFire = 0;
	if(toAccept.Passengers())
		crewToFire = toAccept.Passengers() - player.Cargo().BunksFree();
	if(cargoToSell > 0 || crewToFire > 0)
	{
		if(force)
		{
			MakeSpaceAndAccept();
			return;
		}
		ostringstream out;
		if(cargoToSell > 0 && crewToFire > 0)
			out << "You must fire " << crewToFire << " of your flagship's non-essential crew members and sell "
				<< Format::CargoString(cargoToSell, "ordinary commodities") << " to make room for this mission. Continue?";
		else if(crewToFire > 0)
			out << "You must fire " << crewToFire
				<< " of your flagship's non-essential crew members to make room for this mission. Continue?";
		else
			out << "You must sell " << Format::CargoString(cargoToSell, "ordinary commodities")
				<< " to make room for this mission. Continue?";
		GetUI()->Push(new Dialog(this, &MissionPanel::MakeSpaceAndAccept, out.str()));
		return;
	}

	++availableIt;
	player.AcceptJob(toAccept, GetUI());

	cycleInvolvedIndex = 0;

	if(available.empty())
		return;

	if(availableIt == available.end())
		--availableIt;

	// Check if any other jobs are available with the same destination. Prefer
	// jobs that also have the same destination planet.
	if(toAccept.Destination())
	{
		const Planet *planet = toAccept.Destination();
		const System *system = planet->GetSystem();
		bool stillLooking = true;

		// Updates availableIt if matching system found, returns true if planet also matches.
		auto SelectNext = [this, planet, system, &stillLooking](const list<Mission>::const_iterator &it) -> bool
		{
			if(it->Destination() && it->Destination()->IsInSystem(system))
			{
				if(it->Destination() == planet)
				{
					availableIt = it;
					return true;
				}
				else if(stillLooking)
				{
					stillLooking = false;
					availableIt = it;
				}
			}
			return false;
		};

		const list<Mission>::const_iterator startHere = availableIt;
		for(auto it = startHere; it != available.end(); ++it)
			if(SelectNext(it))
				return;
		for(auto it = startHere; it != available.begin(); )
			if(SelectNext(--it))
				return;
	}
}



void MissionPanel::MakeSpaceAndAccept()
{
	const Mission &toAccept = *availableIt;
	int cargoToSell = toAccept.CargoSize() - player.Cargo().Free();
	int crewToFire = toAccept.Passengers() - player.Cargo().BunksFree();

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
		cargoToSell -= toSell;
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
		player.RemoveMission(Mission::ABORT, toAbort, GetUI());
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
		// No previous selected mission, so select the first job/mission for any system.
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



void MissionPanel::CycleInvolvedSystems(const Mission &mission)
{
	cycleInvolvedIndex++;

	int index = 0;
	for(const System *waypoint : mission.Waypoints())
		if(++index == cycleInvolvedIndex)
		{
			CenterOnSystem(waypoint);
			return;
		}

	for(const Planet *stopover : mission.Stopovers())
		if(++index == cycleInvolvedIndex)
		{
			CenterOnSystem(stopover->GetSystem());
			return;
		}

	for(const System *mark : mission.MarkedSystems())
		if(++index == cycleInvolvedIndex)
		{
			CenterOnSystem(mark);
			return;
		}

	for(const System *mark : mission.TrackedSystems())
		if(++index == cycleInvolvedIndex)
		{
			CenterOnSystem(mark);
			return;
		}

	cycleInvolvedIndex = 0;
	CenterOnSystem(mission.Destination()->GetSystem());
}
