/* MapPlanet.cpp
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MAP_PLANET_CARD_H_
#define MAP_PLANET_CARD_H_

#include "Sprite.h"

#include <vector>
#include <string>

class Point;
class StellarObject;



// Used to manage the display of a single planet in the MapDetailPanel.
class MapPlanetCard {
public:
	// The object HAS to be a planet.
	explicit MapPlanetCard(const StellarObject &object, bool hasVisited);
	~MapPlanetCard();
	// Return if this one was clicked, whether or not we did something about it.
	// Should it return the need to go to the outfitter/shipyard?
	bool Click(int x, int y, int clicks);
	// Draw this at the corresponding scoll; if it is not outside bounds, and return if we drew it.
	bool DrawIfFits(const Point &uiPoint);
	// If this object is currently being shown.
	bool Shown() const;
	double AvailableSpace() const;


public:
	static void setScroll(double newScroll);
	static double getScroll();
	static void clear();


protected:
	// Highlight this card; to be called when it is selected.
	void Highlight(double availableSpace) const;
	double AvailableBottomSpace() const;
	double AvailableTopSpace() const;


private:
	unsigned number;
	bool isSelected = false;

	bool hasVisited;
	bool hasSpaceport;
	bool hasOutfitter;
	bool hasShipyard;

	// The current starting y position.
	double yCoordinate;
	bool isShown = false;
	
	const Sprite *sprite;
	float spriteScale;

	std::string reputationLabel;
	const std::string &planetName;
	// The currently select category (outfitter, shipyard, ...)
	unsigned selectedCategory = 0;


private:
	static double scroll;
	// Used so we can give the right number to the planet cards in an internal way.
	static std::vector<MapPlanetCard *> cards;
};



#endif
