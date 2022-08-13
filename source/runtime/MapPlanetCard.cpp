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
#include "MapDetailPanel.h"
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



MapPlanetCard::MapPlanetCard(const StellarObject &object, unsigned number, bool hasVisited)
	: number(number), hasVisited(hasVisited), planetName(object.Name())
{
	planet = object.GetPlanet();
	hasSpaceport = planet->HasSpaceport();
	hasShipyard = planet->HasShipyard();
	hasOutfitter = planet->HasOutfitter();

	reputationLabel = !hasSpaceport ? "No Spaceport" :
		GameData::GetPolitics().HasDominated(planet) ? "Dominated" :
		planet->GetGovernment()->IsEnemy() ? "Hostile" :
		planet->CanLand() ? "Friendly" : "Restricted";

	sprite = object.GetSprite();

	const Interface* planetCardInterface = GameData::Interfaces().Get("map planet card");
	const float planetIconMaxSize = static_cast<float>(planetCardInterface->GetValue("planet icon max size"));
	spriteScale = min(.5f, min((planetIconMaxSize) / sprite->Width(), (planetIconMaxSize) / sprite->Height()));
}



MapPlanetCard::ClickAction MapPlanetCard::Click(int x, int y, int clicks)
{
	ClickAction clickAction = ClickAction::NONE;
	// The isShown variable should have already updated by the drawing of this item.
	if(isShown)
	{
		const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
		const double textStart = planetCardInterface->GetValue("text start");
		const double categorySize = planetCardInterface->GetValue("category size");
		const double categories = planetCardInterface->GetValue("categories");
		const double planetIconMaxSize = planetCardInterface->GetValue("planet icon max size");

		// The yCoordinate refers to the center of this object.
		double relativeY = y - yCoordinate;
		if(relativeY > 0. && relativeY < AvailableSpace())
		{
			isSelected = true;

			// The first category is the planet name and is not selectable.
			if(x > Screen::Left() + planetIconMaxSize &&
					relativeY > textStart + categorySize && relativeY < textStart + categorySize * categories)
				selectedCategory = (relativeY - textStart - categorySize) / categorySize;
			else
				clickAction = ClickAction::SELECTED;

			static const int SHOW[4] = {MapPanel::SHOW_REPUTATION, MapPanel::SHOW_SHIPYARD,
										MapPanel::SHOW_OUTFITTER, MapPanel::SHOW_GOVERNMENT};
			if(clickAction != ClickAction::SELECTED)
			{
				clickAction = static_cast<ClickAction>(SHOW[selectedCategory]);
				if(clickAction == ClickAction::SHOW_SHIPYARD && clicks > 1)
					clickAction = ClickAction::GOTO_SHIPYARD;
				else if(clickAction == ClickAction::SHOW_OUTFITTER && clicks > 1)
					clickAction = ClickAction::GOTO_OUTFITTER;
			}
		}
	}
	isSelected = clickAction != ClickAction::NONE;
	return clickAction;
}



bool MapPlanetCard::DrawIfFits(const Point &uiPoint)
{
	// Need to update this before checking if the element fits.
	yCoordinate = uiPoint.Y();
	isShown = IsShown();
	if(isShown)
	{
		const Font &font = FontSet::Get(14);
		const Color &faint = *GameData::Colors().Get("faint");
		const Color &dim = *GameData::Colors().Get("dim");
		const Color &medium = *GameData::Colors().Get("medium");
		const auto alignLeft = Layout(140, Truncate::BACK);

		const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
		const double height = planetCardInterface->GetValue("height");
		const double planetIconMaxSize = planetCardInterface->GetValue("planet icon max size");
		const double textStart = planetCardInterface->GetValue("text start");

		const double availableTopSpace = AvailableTopSpace();
		const double availableBottomSpace = AvailableBottomSpace();
		const double categorySize = planetCardInterface->GetValue("category size");
		const double categories = planetCardInterface->GetValue("categories");
		const double categoriesFit = (availableTopSpace - textStart * 2.) / categorySize;

		// If some categories do not fit above we need to draw the last ones at the place where the first ones where.
		double textStartingPosition = textStart - (categories - categoriesFit) * categorySize;

		// We have more leasure at the top if the government sprite is drawn over this element.
		const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
		const double governmentY = mapInterface->GetValue("government top Y");
		const double planetStartingY = mapInterface->GetValue("planet starting Y");
		const double extraLeasure = (governmentY < planetStartingY ? planetStartingY - governmentY : 0.);
		const double planetLowestY = (textStartingPosition - textStart) + height / 2.;
		if(availableTopSpace + extraLeasure >= height / 2. + spriteScale * sprite->Height() / 2. &&
				availableBottomSpace >=  planetLowestY + spriteScale * sprite->Height() / 2.)
			SpriteShader::Draw(sprite, Point(Screen::Left() + planetIconMaxSize / 2.,
				uiPoint.Y() + planetLowestY), spriteScale);

		const auto FitsCategory = [availableTopSpace, availableBottomSpace, textStart, categorySize, height]
			(double number)
		{
			return availableTopSpace >= textStart + categorySize * number &&
				availableBottomSpace >= height - (categorySize * number);
		};

		if(FitsCategory(5.))
			font.Draw({ planetName, alignLeft }, uiPoint + Point(0., textStartingPosition), isSelected ? medium : dim);

		const double margin = mapInterface->GetValue("text margin");
		if(FitsCategory(4.))
			font.Draw(reputationLabel, uiPoint + Point(margin, textStartingPosition + categorySize),
				hasSpaceport ? medium : faint);
		if(FitsCategory(3.))
			font.Draw("Shipyard", uiPoint + Point(margin, textStartingPosition + categorySize * 2.),
				hasShipyard ? medium : faint);
		if(FitsCategory(2.))
			font.Draw("Outfitter", uiPoint + Point(margin, textStartingPosition + categorySize * 3.),
				hasOutfitter ? medium : faint);
		if(FitsCategory(1.))
			font.Draw(hasVisited ? "(has been visited)" : "(not yet visited)",
				uiPoint + Point(margin, textStartingPosition + categorySize * 4.), dim);

		if(FitsCategory(categories - (selectedCategory + 1.)))
			PointerShader::Draw(uiPoint + Point(margin, textStartingPosition + 8. + (selectedCategory + 1) * categorySize),
				Point(1., 0.), 10.f, 10.f, 0.f, medium);

		if(isSelected)
			Highlight(min(availableBottomSpace, availableTopSpace));
	}
	else
		yCoordinate = Screen::Bottom();

	return isShown;
}



bool MapPlanetCard::IsShown() const
{
	return AvailableSpace() > 15.;
}



bool MapPlanetCard::IsSelected() const
{
	return isSelected;
}



double MapPlanetCard::AvailableSpace() const
{
	return min(AvailableBottomSpace(), AvailableTopSpace());
}



const Planet *MapPlanetCard::GetPlanet() const
{
	return planet;
}



void MapPlanetCard::Highlight(double availableSpace) const
{
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	const double width = planetCardInterface->GetValue("width");

	FillShader::Fill(Point(Screen::Left() + width / 2., yCoordinate + availableSpace / 2.),
		Point(width, availableSpace), *GameData::Colors().Get("faint"));
}



double MapPlanetCard::AvailableTopSpace() const
{
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	const double height = planetCardInterface->GetValue("height");

	return min(height, max(0., (number + 1) * height - MapDetailPanel::GetScroll()));
}



double MapPlanetCard::AvailableBottomSpace() const
{
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	double maxBottomY = mapInterface->GetValue("planet max bottom Y");
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	double height = planetCardInterface->GetValue("height");

	return min(height, max(0., (Screen::Bottom() - maxBottomY) - yCoordinate));
}
