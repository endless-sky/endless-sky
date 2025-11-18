/* MapPlanetCard.h
Copyright (c) 2022 by Hurleveur

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

#include <string>

class MapDetailPanel;
class Point;
class Sprite;
class StellarObject;



// Used to manage the display of a single planet in the MapDetailPanel.
class MapPlanetCard {
public:
	enum class ClickAction : int {
		SHOW_GOVERNMENT = MapPanel::SHOW_GOVERNMENT,
		SHOW_REPUTATION = MapPanel::SHOW_REPUTATION,
		SHOW_SHIPYARD = MapPanel::SHOW_SHIPYARD,
		SHOW_OUTFITTER = MapPanel::SHOW_OUTFITTER,
		SHOW_VISITED = MapPanel::SHOW_VISITED,
		GOTO_SHIPYARD = 3,
		GOTO_OUTFITTER = 2,
		SELECTED = 1,
		NONE = 0
	};


public:
	// For the orbit selection to work properly this has to be a planet.
	explicit MapPlanetCard(const StellarObject &object, unsigned number, bool hasVisited, const MapDetailPanel *parent);
	// Return if this one was clicked, whether or not we did something about it.
	ClickAction Click(int x, int y, int clicks);
	// Draw this at the corresponding scroll; if it is not outside bounds, and return if we drew it.
	bool DrawIfFits(const Point &uiPoint);
	// If this object is currently being shown.
	bool IsShown() const;
	// Whether or not this object by selected, by clicking on it or otherwise.
	bool IsSelected() const;
	// Return the space available for this planet card on its current position.
	double AvailableSpace() const;

	const Planet *GetPlanet() const;

	void Select(bool select = true);

	static double Height();

	static void ResetSize();


protected:
	// Highlight this card; this is to be called when it is selected.
	void Highlight(double availableSpace) const;
	double AvailableBottomSpace() const;
	double AvailableTopSpace() const;


private:
	const Planet *planet;
	const MapDetailPanel *parent;

	unsigned number;
	bool isSelected = false;

	bool hasVisited;
	bool hasSpaceport;
	bool hasOutfitter;
	bool hasShipyard;

	// The current starting y position.
	double yCoordinate = 0.;
	bool isShown = false;

	const Sprite *sprite;
	float spriteScale;

	std::string governmentName;
	std::string reputationLabel;
	const std::string &planetName;
	// The currently select category (outfitter, shipyard, ...)
	unsigned selectedCategory = 0;
};
