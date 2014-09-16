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

class Mission;



// A panel that displays a list of missions (accepted missions, and also 
// available missions to accept if any) and a map of their destinations.
class MissionPanel : public MapPanel {
public:
	MissionPanel(PlayerInfo &player);
	MissionPanel(const MapPanel &panel);
	
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod) override;
	virtual bool Click(int x, int y) override;
	virtual bool Drag(int dx, int dy) override;
	
	
private:
	void DrawSelectedSystem() const;
	Point DrawPanel(Point pos, const std::string &label, int entries) const;
	Point DrawList(const std::list<Mission> &list, Point pos) const;
	void DrawMissionInfo() const;
	
	bool CanAccept() const;
	void AbortMission();
	
	
private:
	const std::list<Mission> &available;
	const std::list<Mission> &accepted;
	std::list<Mission>::const_iterator availableIt;
	std::list<Mission>::const_iterator acceptedIt;
	int availableScroll;
	int acceptedScroll;
	
	int dragSide;
	mutable WrappedText wrap;
};



#endif
