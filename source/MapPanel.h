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

#include "Color.h"
#include "DistanceMap.h"
#include "Point.h"

#include <map>
#include <string>

class Angle;
class Government;
class Mission;
class Planet;
class PlayerInfo;
class System;



// This class provides the base class for both the "map details" panel and the
// missions panel, and handles drawing of the underlying starmap and coloring
// the systems based on a selected criterion. It also handles finding and
// drawing routes in between systems.
class MapPanel : public Panel {
public:
	// Enumeration for how the systems should be colored:
	static const int SHOW_SHIPYARD = -1;
	static const int SHOW_OUTFITTER = -2;
	static const int SHOW_VISITED = -3;
	static const int SHOW_SPECIAL = -4;
	static const int SHOW_GOVERNMENT = -5;
	static const int SHOW_REPUTATION = -6;
	
	static const double OUTER;
	static const double INNER;
	
	
public:
	MapPanel(PlayerInfo &player, int commodity = SHOW_REPUTATION, const System *special = nullptr);
	
	void SetCommodity(int index);
	virtual void Step() override;
	virtual void Draw() override;
	
	static void DrawMiniMap(const PlayerInfo &player, double alpha, const System *const jump[2], int step);
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool Click(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;
	
	// Get the color mapping for various system attributes.
	static Color MapColor(double value);
	static Color ReputationColor(double reputation, bool canLand, bool hasDominated);
	static Color GovernmentColor(const Government *government);
	static Color UninhabitedColor();
	static Color UnexploredColor();
	
	virtual double SystemValue(const System *system) const;
	
	void Select(const System *system);
	const Planet *Find(const std::string &name);
	
	void ZoomMap();
	void UnzoomMap();
	double Zoom() const;
	bool ZoomIsMax() const;
	bool ZoomIsMin() const;
	
	// Check whether the NPC and waypoint conditions of the given mission have
	// been satisfied.
	bool IsSatisfied(const Mission &mission) const;
	static bool IsSatisfied(const PlayerInfo &player, const Mission &mission);
	
	// Function for the "find" dialogs:
	static int Search(const std::string &str, const std::string &sub);
	
	
protected:
	PlayerInfo &player;
	
	DistanceMap distance;
	
	const System *playerSystem;
	const System *selectedSystem;
	const System *specialSystem;
	
	Point center;
	int commodity;
	int *tradeCommodity = nullptr;
	const int maxZoom = 2;
	int zoom = 0;
	mutable int step = 0;
	
	mutable std::map<const Government *, double> closeGovernments;
	
	
private:
	void DrawTravelPlan() const;
	void DrawWormholes() const;
	void DrawLinks() const;
	void DrawSystems() const;
	void DrawNames() const;
	void DrawMissions() const;
	void DrawPointer(const System *system, Angle &angle, const Color &color, bool bigger = false) const;
	static void DrawPointer(Point position, Angle &angle, const Color &color, bool drawBack = true, bool bigger = false);
};



#endif
