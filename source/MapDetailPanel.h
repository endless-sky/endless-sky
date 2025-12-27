/* MapDetailPanel.h
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

#include "ClickZone.h"
#include "MapPlanetCard.h"
#include "Point.h"
#include "ScrollBar.h"
#include "ScrollVar.h"

#include <map>
#include <vector>

class Planet;
class PlayerInfo;
class System;
class TextArea;



// A panel that displays the galaxy star map, with options for color-coding the
// stars based on attitude towards the player, government, or commodity price.
// This panel also lets you view what planets are in each system, and you can
// click on a planet to view its description.
class MapDetailPanel : public MapPanel {
public:
	explicit MapDetailPanel(PlayerInfo &player, const System *system = nullptr, bool fromMission = false);
	explicit MapDetailPanel(const MapPanel &panel, bool isStars);

	virtual void Step() override;
	virtual void Draw() override;

	double GetScroll() const;

public:
	static double PlanetPanelHeight();


protected:
	// Navigates through the shown planets panel, and drags them around.
	virtual bool Scroll(double dx, double dy) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;

	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	// Handle single & double-clicks on commodities, planet information, or objects in the "orbits" display.
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;

	virtual void Resize() override;


private:
	void InitTextArea();
	void ResizeTextArea();
	void GeneratePlanetCards(const System &system);
	void DrawKey();
	void DrawInfo();
	void DrawOrbits();

	// Set the commodity coloring, and update the player info as well.
	void SetCommodity(int index);


private:
	int governmentY = 0;
	int tradeY = 0;

	// Which panel is being hovered over and should be affected by up and down keys.
	bool isPlanetViewSelected = false;
	bool isStars = false;

	ScrollVar<double> scroll;
	ScrollBar scrollbar;

	// Default display scaling for orbits within the currently displayed system.
	double scale = .03;

	// The system currently displayed, it should be the same as the system selected at all times.
	const System *shownSystem = nullptr;

	static double planetPanelHeight;
	std::vector<MapPlanetCard> planetCards;
	// Vector offsets from the center of the "orbits" UI.
	std::map<const Planet *, Point> planets;

	std::shared_ptr<TextArea> description = nullptr;
	bool descriptionVisible = false;
	int descriptionXOffset;

	std::vector<ClickZone<int>> clickZones;
};
