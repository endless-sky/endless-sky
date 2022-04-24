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

#include "Panel.h"
#include "Sprite.h"

#include <vector>
#include <string>

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
	// Draw this at the corresponding scoll; if it is not outside bounds.
	void Draw() const;


public:
	static void setScroll(double newScroll);
	static double getScroll();


private:
	void Highlight() const;


private:
	unsigned number;
	bool isSelected;
	bool hasVisited;
	bool hasSpaceport;
	bool hasOutfitter;
	bool hasShipyard;
	const Sprite *sprite;
	std::string reputationLabel;
	const std::string &planetName;
	// The currently select category (outfitter, shipyard, ...)
	unsigned selectedCategory = 0;


private:
	static double scroll;
	// Used to make managing selection an internal affair.
	static std::vector<MapPlanetCard *> cards;
};



#endif
