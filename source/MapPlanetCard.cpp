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
	const double CATEGORY_SIZE = 20.;
	const double CATEGORY_START = 33.;
	const double CATEGORY_END = 15.;
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
	const double height = mapInterface->GetValue("one planet display height");
	// The yCoordinate refers to the center of this object.
	double relativeY = y - yCoordinate + height / 2.;
	if(relativeY > 0. && relativeY < height)
	{
		isSelected = true;
		
		if(relativeY > CATEGORY_START && relativeY < height - CATEGORY_END)
			selectedCategory = (relativeY - CATEGORY_START) / CATEGORY_SIZE;
	} else
		isSelected = false;
	return isSelected;
}



bool MapPlanetCard::Draw(Point &uiPoint)
{
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	double height = mapInterface->GetValue("one planet display height");
	int scrollInt = static_cast<int>(getScroll());
	int heightInt = static_cast<int>(height);
	double maxBottomY = mapInterface->GetValue("planet max bottom Y");
	
	// The margins are to allow a half drawn planet card. For the conditions it first looks the upper and then the lower bounds.
	if((getScroll() - height / 2.) / height <= number &&	
		uiPoint.Y() + scrollInt % heightInt < 
		Screen::Bottom() - maxBottomY + heightInt / 2)
	{
		const Font &font = FontSet::Get(14);
		const Color &faint = *GameData::Colors().Get("faint");
		const Color &dim = *GameData::Colors().Get("dim");
		const Color &medium = *GameData::Colors().Get("medium");
		const auto alignLeft = Layout(140, Truncate::BACK);

		yCoordinate = uiPoint.Y();

		// uiPoint is the center of this element, we want the relative distance to the top/bottom.
		const double halfHeight = mapInterface->GetValue("one planet display height") / 2.;
		const int planetIconMaxSize = static_cast<int>(mapInterface->GetValue("planet icon max size"));

		double scale = min(.5f, min((planetIconMaxSize) / sprite->Width(), (planetIconMaxSize) / sprite->Height()));
		SpriteShader::Draw(sprite, Point(Screen::Left() + planetIconMaxSize / 2, uiPoint.Y()), scale);

		font.Draw({ planetName, alignLeft }, uiPoint + Point(0., -52.), isSelected ? medium : dim);

		double margin = 10.;
		font.Draw(reputationLabel, uiPoint + Point(margin, -halfHeight + CATEGORY_START), hasSpaceport ? medium : faint);
		font.Draw("Shipyard", uiPoint + Point(margin, -halfHeight + CATEGORY_START + CATEGORY_SIZE), hasShipyard ? medium : faint);
		font.Draw("Outfitter", uiPoint + Point(margin, -halfHeight + CATEGORY_START + CATEGORY_SIZE * 2.), hasOutfitter ? medium : faint);
		font.Draw(hasVisited ? "(has been visited)" : "(not yet visited)", uiPoint + Point(margin, -halfHeight + CATEGORY_START + CATEGORY_SIZE * 3.), dim);

		PointerShader::Draw(uiPoint + Point(margin, -halfHeight + CATEGORY_START + 8. + selectedCategory * CATEGORY_SIZE), Point(1., 0.), 10.f, 10.f, 0.f, medium);

		if(isSelected)
			Highlight(uiPoint);
		
		uiPoint.Y() += height;
	} else
		yCoordinate = 0.;

	return yCoordinate;
}



void MapPlanetCard::setScroll(double newScroll)
{
	MapPlanetCard::scroll = newScroll;
}



double MapPlanetCard::getScroll()
{
	return MapPlanetCard::scroll;
}



void MapPlanetCard::clear()
{
	cards.clear();
}



void MapPlanetCard::Highlight(const Point &uiPoint) const
{
	const Color &faint = *GameData::Colors().Get("faint");

	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	const double height = mapInterface->GetValue("one planet display height");
	const double width = mapInterface->GetValue("planet display width");

	FillShader::Fill(uiPoint, Point(width, height), faint);
}
