/* MissionPanel.h
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

#pragma once

#include "MapPanel.h"

#include <list>

class Color;
class Interface;
class Mission;
class PlayerInfo;
class TextArea;



// A panel that displays a list of missions (accepted missions, and also the
// available "jobs" to accept if any) and a map of their destinations. You can
// accept any "jobs" that are available, and can also abort any mission that you
// have previously accepted.
class MissionPanel : public MapPanel {
public:
	explicit MissionPanel(PlayerInfo &player);
	explicit MissionPanel(const MapPanel &panel);

	virtual void Step() override;
	virtual void Draw() override;

	virtual void UpdateTooltipActivation() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;

	virtual void Resize() override;


private:
	void InitTextArea();
	void ResizeTextArea() const;
	// Use availableIt/acceptedIt to set MapPanel::selectedSystem, call DoScroll/CenterOnSystem.
	// CenterOnSystem will either pan to the system or immediately jump to it.
	void SetSelectedScrollAndCenter(bool immediate = false);
	// Display and explain the various pointers that may appear on the map.
	void DrawKey() const;
	// Draw rings around systems that need to be visited for the given mission.
	void DrawMissionSystem(const Mission &mission, const Color &color) const;
	// Draw the backgrounds for the "available jobs" and accepted missions/jobs lists.
	Point DrawPanel(Point pos, const std::string &label, int entries, bool sorter = false) const;
	// Draw the display names of the given missions, using the reference point.
	Point DrawList(const std::list<Mission> &missionList, Point pos, const std::list<Mission>::const_iterator &selectIt,
		bool separateDeadlineOrPossible = false) const;
	void DrawMissionInfo();
	void DrawTooltips();

	bool CanAccept() const;
	void Accept(bool force = false);
	void MakeSpaceAndAccept();
	void AbortMission();

	int AcceptedVisible() const;

	// Updates availableIt and acceptedIt to select the first available or
	// accepted mission in the given system. Returns true if a mission was found.
	bool FindMissionForSystem(const System *system);
	// Selects the first available or accepted mission if no mission is already
	// selected. Returns true if the selection was changed.
	bool SelectAnyMission();
	// Centers on the next involved system for the clicked mission from the mission list
	void CycleInvolvedSystems(const Mission &mission);


private:
	const Interface *missionInterface;

	const std::list<Mission> &available;
	const std::list<Mission> &accepted;
	int cycleInvolvedIndex = 0;
	std::list<Mission>::const_iterator availableIt;
	std::list<Mission>::const_iterator acceptedIt;
	double availableScroll = 0.;
	double acceptedScroll = 0.;

	bool canDrag = true;

	int dragSide = 0;

	// 0 to 3 for each UI element
	int hoverSort = -1;
	mutable Tooltip tooltip;

	std::shared_ptr<TextArea> description;
	bool descriptionVisible = false;
};
