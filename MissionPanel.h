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

#include "Panel.h"

#include "DistanceMap.h"
#include "Point.h"
#include "WrappedText.h"

#include <set>
#include <list>

class GameData;
class Mission;
class PlayerInfo;
class System;



// A panel that displays a list of missions (accepted missions, and also 
// available missions to accept if any) and a map of their destinations.
class MissionPanel : public Panel {
public:
	MissionPanel(const GameData &data, PlayerInfo &player);
	
	virtual void Draw() const;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	
	
private:
	void DrawMap() const;
	void DrawSelectedSystem() const;
	void DrawList(const std::list<Mission> &list, Point pos, const std::string &label) const;
	void DrawMissionInfo() const;
	
	void Select(const System *system);
	bool CanAccept() const;
	
	
private:
	const GameData &data;
	const System *playerSystem;
	const System *selectedSystem;
	PlayerInfo &player;
	
	DistanceMap distance;
	std::set<const System *> destinations;
	
	const std::list<Mission> &available;
	const std::list<Mission> &accepted;
	std::list<Mission>::const_iterator availableIt;
	std::list<Mission>::const_iterator acceptedIt;
	int availableScroll;
	int acceptedScroll;
	
	int dragSide;
	Point center;
	mutable WrappedText wrap;
};



#endif
