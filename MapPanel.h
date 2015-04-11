/* MapPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MAP_PANEL_H_
#define MAP_PANEL_H_

#include "Panel.h"

#include "DistanceMap.h"
#include "Point.h"

#include <set>
#include <string>

class Planet;
class PlayerInfo;
class System;



// This class provides the base class for both the "map details" panel and the
// missions panel, and handles drawing of the underlying starmap and coloring
// the systems based on a selected criterion. It also handles finding and
// drawing routes in between systems.
class MapPanel : public Panel {
public:
	MapPanel(PlayerInfo &player, int commodity = -4, const System *special = nullptr);
	
	virtual void Draw() const override;
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool Click(int x, int y) override;
	virtual bool Drag(int dx, int dy) override;
	
	void Select(const System *system);
	const Planet *Find(const std::string &name);
	
	
protected:
	PlayerInfo &player;
	
	DistanceMap distance;
	
	const System *playerSystem;
	const System *selectedSystem;
	const System *specialSystem;
	
	Point center;
	int commodity;
	
	
private:
	void DrawTravelPlan() const;
	void DrawLinks() const;
	void DrawSystems() const;
	void DrawNames() const;
	void DrawMissions() const;
};



#endif
