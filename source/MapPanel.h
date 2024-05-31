/* MapPanel.h
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

#ifndef MAP_PANEL_H_
#define MAP_PANEL_H_

#include "Panel.h"

#include "Color.h"
#include "DistanceMap.h"
#include "Point.h"
#include "text/WrappedText.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

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
	static const int SHOW_DANGER = -7;

	static const float OUTER;
	static const float INNER;
	static const float LINK_WIDTH;
	static const float LINK_OFFSET;

	class SystemTooltipData {
	public:
		// Number of ships that are in flight
		unsigned activeShips = 0;
		// Number of ships that are parked
		unsigned parkedShips = 0;
		// Maps planet to number of outfits on that planet
		std::map<const Planet *, unsigned> outfits;
	};



public:
	explicit MapPanel(PlayerInfo &player, int commodity = SHOW_REPUTATION, const System *special = nullptr);

	virtual void Step() override;
	virtual void Draw() override;

	// Draw elements common for all map panels that need to be placed
	// on top of everything else. This includes distance info, map mode buttons,
	// escort/storage tooltips, and the non-routable system warning.
	void FinishDrawing(const std::string &buttonCondition);

	static void DrawMiniMap(const PlayerInfo &player, float alpha, const System *const jump[2], int step);

	// Map panels allow fast-forward to stay active.
	bool AllowsFastForward() const noexcept final;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;

	// Get the color mapping for various system attributes.
	static Color MapColor(double value);
	static Color ReputationColor(double reputation, bool canLand, bool hasDominated);
	static Color GovernmentColor(const Government *government);
	static Color DangerColor(double danger);
	static Color UninhabitedColor();
	static Color UnexploredColor();

	virtual double SystemValue(const System *system) const;

	void Select(const System *system);
	void Find(const std::string &name);

	double Zoom() const;

	// Check whether the NPC and waypoint conditions of the given mission have
	// been satisfied.
	bool IsSatisfied(const Mission &mission) const;
	static bool IsSatisfied(const PlayerInfo &player, const Mission &mission);

	// Returns if previous->next can be done with a known travel type.
	bool GetTravelInfo(const System *previous, const System *next, double jumpRange, bool &isJump,
		bool &isWormhole, bool &isMappable, Color *wormholeColor) const;


protected:
	PlayerInfo &player;

	DistanceMap distance;

	// The system in which the player is located.
	const System &playerSystem;
	// The (non-null) system which is currently selected.
	const System *selectedSystem;
	// The selected planet, if any.
	const Planet *selectedPlanet = nullptr;
	// A system associated with a dialog or conversation.
	const System *specialSystem;

	double playerJumpDistance;

	Point center;
	Point recenterVector;
	int recentering = 0;
	int commodity;
	int step = 0;
	std::string buttonCondition;

	// Distance from the screen center to the nearest owned system,
	// for use in determining which governments are in the legend.
	std::map<const Government *, double> closeGovernments;
	// Systems in which your (active and parked) escorts and stored outfits are located.
	std::map<const System *, SystemTooltipData> escortSystems;
	// Center the view on the given system (may actually be slightly offset
	// to account for panels on the screen).
	void CenterOnSystem(const System *system, bool immediate = false);

	// Cache the map layout, so it doesn't have to be re-calculated every frame.
	// The cache must be updated when the coloring mode changes.
	void UpdateCache();

	// For tooltips:
	int hoverCount = 0;
	const System *hoverSystem = nullptr;
	std::string tooltip;
	WrappedText hoverText;

private:
	void DrawTravelPlan();
	// Display the name of and distance to the selected system.
	void DrawSelectedSystem();
	// Indicate which other systems have player escorts.
	void DrawEscorts();
	void DrawWormholes();
	void DrawLinks();
	// Draw systems in accordance to the set commodity color scheme.
	void DrawSystems();
	void DrawNames();
	void DrawMissions();
	void DrawPointer(const System *system, unsigned &systemCount, const Color &color, bool bigger = false);
	static void DrawPointer(Point position, unsigned &systemCount, const Color &color,
		bool drawBack = true, bool bigger = false);


private:
	// This is the coloring mode currently used in the cache.
	int cachedCommodity = -10;

	class Node {
	public:
		Node(const Point &position, const Color &color, const std::string &name,
			const Color &nameColor, const Government *government)
			: position(position), color(color), name(name), nameColor(nameColor), government(government) {}

		Point position;
		Color color;
		std::string name;
		Color nameColor;
		const Government *government;
	};
	std::vector<Node> nodes;

	class Link {
	public:
		Link(const Point &start, const Point &end, const Color &color)
			: start(start), end(end), color(color) {}

		Point start;
		Point end;
		Color color;
	};
	std::vector<Link> links;
};



#endif
