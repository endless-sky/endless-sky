/* MissionPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MISSION_PANEL_H_
#define MISSION_PANEL_H_

#include "MapPanel.h"

#include "WrappedText.h"

#include <list>

class Color;
class Mission;



// A panel that displays a list of missions (accepted missions, and also the
// available "jobs" to accept if any) and a map of their destinations. You can
// accept any "jobs" that are available, and can also abort any mission that you
// have previously accepted.
class MissionPanel : public MapPanel {
public:
	MissionPanel(PlayerInfo &player);
	MissionPanel(const MapPanel &panel);
	
	virtual void Step() override;
	virtual void Draw() override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command) override;
	virtual bool Click(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;
	
	
private:
	void DoFind(const std::string &text);
	
	void DrawKey() const;
	void DrawSelectedSystem() const;
	void DrawMissionSystem(const Mission &mission, const Color &color) const;
	Point DrawPanel(Point pos, const std::string &label, int entries) const;
	Point DrawList(const std::list<Mission> &list, Point pos) const;
	void DrawMissionInfo();
	
	bool CanAccept() const;
	void Accept();
	void MakeSpaceAndAccept();
	void AbortMission();
	
	int AcceptedVisible() const;
	
	// Updates availableIt and acceptedIt to select the first available or
	// accepted mission in the given system. Returns true if a mission was found.
	bool FindMissionForSystem(const System *system);
	// Selects the first available or accepted mission if no mission is already
	// selected. Returns true if the selection was changed.
	bool SelectAnyMission();
	
private:
	const std::list<Mission> &available;
	const std::list<Mission> &accepted;
	std::list<Mission>::const_iterator availableIt;
	std::list<Mission>::const_iterator acceptedIt;
	double availableScroll = 0.;
	double acceptedScroll = 0.;
	
	int dragSide = 0;
	mutable WrappedText wrap;
};



#endif
