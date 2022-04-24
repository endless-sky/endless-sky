/* MapPlanet.cpp
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapPlanetCard.h"

#include "Color.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "Point.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Planet.h"
#include "Interface.h"
#include "Screen.h"
#include "SpriteShader.h"
#include "StellarObject.h"
#include "text/WrappedText.h"

using namespace std;

namespace {
	// The size of one category (outfitter, shipyard, ...)
	const double CATEGORY_SIZE = 30.;
}
vector<MapPlanetCard *> MapPlanetCard::cards{};
double MapPlanetCard::scroll = 0.;



MapPlanetCard::MapPlanetCard(const StellarObject &object, bool hasVisited) : hasVisited(hasVisited), planetName(object.Name())
{
	number = cards.size();
	cards.emplace_back(this);
	const Planet *planet = object.GetPlanet();
	hasSpaceport = planet->HasSpaceport();
	hasShipyard = planet->HasShipyard();
	hasOutfitter = planet->HasOutfitter();
	
	reputationLabel = !hasSpaceport ? "No Spaceport" :
		GameData::GetPolitics().HasDominated(planet) ? "Dominated" :
		planet->GetGovernment()->IsEnemy() ? "Hostile" :
		planet->CanLand() ? "Friendly" : "Restricted";
	
	sprite = object.GetSprite();
}



MapPlanetCard::~MapPlanetCard()
{
	auto begin = cards.begin();
	bool found = false;
	unsigned i = 0;
	for(auto &it : cards)
	{
		// The ones after the one we deleted need their number adjusted.
		if(found)
			--it->number;
		else if(it == this)
			found = true;
		else
			++i;
	}
	// We should be in it, but just to be sure.
	if(found)
		cards.erase(begin + i);
}


bool MapPlanetCard::Click(int x, int y, int clicks)
{
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	const double width = mapInterface->GetValue("planet display width");
	const double startingY = mapInterface->GetValue("planet start y");
	// This would only get called if a click happens within bounds, and stops when we find the one clicked,
	// so we don't need to check if we are in the bounds.
	double relativeY = y - (Screen::Top() + startingY + number * width);
	if(relativeY > 0. && relativeY < width)
	{
		// Unselect everyone.
		for(auto card: cards)
			card->isSelected = false;
		isSelected = true;
		
		selectedCategory = relativeY / CATEGORY_SIZE;
		return true;
	}
	return false;
}



void MapPlanetCard::Draw() const
{
	const Font &font = FontSet::Get(14);
	const Color &faint = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const auto alignLeft = Layout(140, Truncate::BACK);

	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	const double height = mapInterface->GetValue("one planet display height");
	const double startingY = mapInterface->GetValue("planet start y");
	const int planetIconMaxSize = static_cast<int>(mapInterface->GetValue("planet icon max size"));
	
	Point position = Point(100., startingY + height * number);

	double scale = min(.5f, min((planetIconMaxSize) / sprite->Width(), (planetIconMaxSize) / sprite->Height()));
	SpriteShader::Draw(sprite, Point(Screen::Left() + planetIconMaxSize / 2, position.Y()), scale);

	font.Draw({ planetName, alignLeft }, position + Point(0., -52.),	isSelected ? medium : dim);

	font.Draw(reputationLabel, position + Point(10., -32.), hasSpaceport ? medium : faint);
	font.Draw("Shipyard", position + Point(10., -12.), hasShipyard ? medium : faint);
	font.Draw("Outfitter", position + Point(10., 8.), hasOutfitter ? medium : faint);
	font.Draw(hasVisited ? "(has been visited)" : "(not yet visited)", position + Point(10., 28.), dim);

	PointerShader::Draw(position + Point(0. + 10., -25. + selectedCategory * 20.), Point(1., 0.), 10.f, 10.f, 0.f, medium);

	if(isSelected)
		Highlight();
}



void MapPlanetCard::setScroll(double newScroll)
{
	MapPlanetCard::scroll = newScroll;
}



double MapPlanetCard::getScroll()
{
	return MapPlanetCard::scroll;
}



void MapPlanetCard::Highlight() const
{
	const Color &faint = *GameData::Colors().Get("faint");

	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	const double height = mapInterface->GetValue("one planet display height");
	const double width = mapInterface->GetValue("planet display width");
	const double startingY = mapInterface->GetValue("planet start y");

	FillShader::Fill(Point(Screen::Left() + height - 20., startingY + height * number), Point(width, height), faint);
}
