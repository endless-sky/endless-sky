/* MapPlanetCard.cpp
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

#include "MapPlanetCard.h"

#include "Color.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "Planet.h"
#include "Point.h"
#include "shader/PointerShader.h"
#include "Screen.h"
#include "image/Sprite.h"
#include "shader/SpriteShader.h"
#include "StellarObject.h"
#include "System.h"
#include "text/WrappedText.h"

using namespace std;

namespace {
	bool hasGovernments = false;
}



MapPlanetCard::MapPlanetCard(const StellarObject &object, unsigned number, bool hasVisited,
		const MapDetailPanel *parent)
	: parent(parent), number(number), hasVisited(hasVisited), planetName(object.DisplayName())
{
	planet = object.GetPlanet();
	hasSpaceport = planet->HasServices();
	hasShipyard = planet->HasShipyard();
	hasOutfitter = planet->HasOutfitter();
	governmentName = planet->GetGovernment()->DisplayName();
	string systemGovernmentName = planet->GetSystem()->GetGovernment()->DisplayName();
	if(governmentName != "Uninhabited" && governmentName != systemGovernmentName)
		hasGovernments = true;

	if(!hasSpaceport)
		reputationLabel = "No Spaceport";
	else
	{
		switch(planet->GetFriendliness())
		{
			case Planet::Friendliness::FRIENDLY:
				reputationLabel = "Friendly";
				break;
			case Planet::Friendliness::RESTRICTED:
				reputationLabel = "Restricted";
				break;
			case Planet::Friendliness::HOSTILE:
				reputationLabel = "Hostile";
				break;
			case Planet::Friendliness::DOMINATED:
				reputationLabel = "Dominated";
				break;
		}
	}

	sprite = object.GetSprite();

	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	const float planetIconMaxSize = static_cast<float>(planetCardInterface->GetValue("planet icon max size"));
	spriteScale = min(.5f, min(planetIconMaxSize / sprite->Width(), planetIconMaxSize / sprite->Height()));
}



MapPlanetCard::ClickAction MapPlanetCard::Click(int x, int y, int clicks)
{
	ClickAction clickAction = ClickAction::NONE;
	// The isShown variable should have already updated by the drawing of this item.
	if(isShown)
	{
		const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
		// Point at which the text starts (after the top margin), at first there is the planet's name,
		// and then it is divided into clickable categories of the same size.
		const double textStart = planetCardInterface->GetValue("text start");
		const double categorySize = planetCardInterface->GetValue("category size");
		const double categories = planetCardInterface->GetValue("categories");
		// The maximum possible size for the sprite of the planet.
		const double planetIconMaxSize = planetCardInterface->GetValue("planet icon max size");

		// The yCoordinate refers to the center of this object.
		double relativeY = (y - yCoordinate);
		if(relativeY > 0. && relativeY < AvailableSpace())
		{
			// The first category is the planet name and is not selectable.
			if(x > Screen::Left() + planetIconMaxSize &&
					relativeY > textStart + categorySize && relativeY < textStart + categorySize * (categories + hasGovernments))
				selectedCategory = (relativeY - textStart - categorySize) / categorySize;
			else
				clickAction = ClickAction::SELECTED;

			static const int SHOW[5] = {MapPanel::SHOW_GOVERNMENT, MapPanel::SHOW_REPUTATION,
										MapPanel::SHOW_SHIPYARD, MapPanel::SHOW_OUTFITTER,
										MapPanel::SHOW_VISITED};
			if(clickAction != ClickAction::SELECTED)
			{
				// If there are no governments shown, the first category is the reputation.
				clickAction = static_cast<ClickAction>(SHOW[selectedCategory + !hasGovernments]);
				// Double clicking results in going to the shipyard/outfitter.
				if(clickAction == ClickAction::SHOW_SHIPYARD && clicks > 1)
					clickAction = ClickAction::GOTO_SHIPYARD;
				else if(clickAction == ClickAction::SHOW_OUTFITTER && clicks > 1)
					clickAction = ClickAction::GOTO_OUTFITTER;
			}
		}
	}
	isSelected = (clickAction != ClickAction::NONE);
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

		const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
		// The maximum possible size for the sprite of the planet.
		const double planetIconMaxSize = planetCardInterface->GetValue("planet icon max size");
		const auto alignLeft = Layout(planetCardInterface->GetValue("width") - planetIconMaxSize, Truncate::BACK);

		// Height of one MapPlanetCard element.
		const double height = Height();
		// Point at which the text starts (after the top margin), at first there is the planet's name,
		// and then it is divided into clickable categories of the same size.
		const double textStart = planetCardInterface->GetValue("text start");
		const double categorySize = planetCardInterface->GetValue("category size");
		const double categories = planetCardInterface->GetValue("categories");

		// Available space, limited by the space there is between the top of this item,
		// and the end of the panel below.
		const double availableBottomSpace = AvailableBottomSpace();

		// The top part goes out of the screen so we can draw there. The bottom would go out of this panel.
		const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");

		auto spriteItem = SpriteShader::Prepare(sprite, Point(Screen::Left() + planetIconMaxSize / 2.,
			uiPoint.Y() + height / 2.), spriteScale);

		float clip = 1.f;
		// Lowest point of the planet sprite.
		double planetBottomY = height / 2. + spriteScale * sprite->Height() / 2.;
		// Calculate the correct clip on the bottom of the sprite if necessary.
		// It is done by looking at how much space is available,
		// and the difference between that and the lowest point of the sprite.
		// Of course, the clipping needs to be done relative to the size of the sprite.
		if(availableBottomSpace <= planetBottomY)
			clip = 1.f + (availableBottomSpace - planetBottomY) / (spriteScale * sprite->Height());

		spriteItem.clip = clip;
		spriteItem.position[1] -= (sprite->Height() * ((1.f - clip) * .5f)) * spriteScale;
		spriteItem.transform[3] *= clip;

		SpriteShader::Bind();
		SpriteShader::Add(spriteItem);
		SpriteShader::Unbind();

		// Check if drawing a category would not go out of the panel.
		const auto FitsCategory = [availableBottomSpace, categorySize, height]
			(double number)
		{
			return availableBottomSpace >= height - (categorySize * number);
		};

		// Draw the name of the planet.
		if(FitsCategory(categories + hasGovernments))
			font.Draw({ planetName, alignLeft }, uiPoint + Point(0, textStart), isSelected ? medium : dim);

		// Draw the government name, reputation, shipyard, outfitter and visited.
		const double margin = mapInterface->GetValue("text margin");
		if(hasGovernments && FitsCategory(categories))
			font.Draw(governmentName, uiPoint + Point(margin, textStart + categorySize),
				governmentName == "Uninhabited" ? faint : dim);
		if(FitsCategory(4.))
			font.Draw(reputationLabel, uiPoint + Point(margin, textStart + categorySize * (1. + hasGovernments)),
				hasSpaceport ? medium : faint);
		if(FitsCategory(3.))
			font.Draw("Shipyard", uiPoint + Point(margin, textStart + categorySize * (2. + hasGovernments)),
				hasShipyard ? medium : faint);
		if(FitsCategory(2.))
			font.Draw("Outfitter", uiPoint + Point(margin, textStart + categorySize * (3. + hasGovernments)),
				hasOutfitter ? medium : faint);
		if(FitsCategory(1.))
			font.Draw(hasVisited ? "(has been visited)" : "(not yet visited)",
				uiPoint + Point(margin, textStart + categorySize * (4. + hasGovernments)), dim);

		// Draw the arrow pointing to the selected category.
		if(FitsCategory(categories - (selectedCategory + 1.)))
			PointerShader::Draw(uiPoint + Point(margin, textStart + 8. + (selectedCategory + 1) * categorySize),
				Point(1., 0.), 10.f, 10.f, 0.f, medium);

		if(isSelected)
			Highlight(availableBottomSpace);
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



void MapPlanetCard::Select(bool select)
{
	isSelected = select;
}



double MapPlanetCard::Height()
{
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	return planetCardInterface->GetValue("height padding") +
		(planetCardInterface->GetValue("categories") + hasGovernments) *
		planetCardInterface->GetValue("category size");
}



void MapPlanetCard::ResetSize()
{
	hasGovernments = false;
}



void MapPlanetCard::Highlight(double availableSpace) const
{
	const Interface *planetCardInterface = GameData::Interfaces().Get("map planet card");
	const double width = planetCardInterface->GetValue("width");

	Rectangle highlightRegion = Rectangle::FromCorner(Point(Screen::Left(), yCoordinate), Point(width, availableSpace));
	FillShader::Fill(highlightRegion, *GameData::Colors().Get("item selected"));
}



double MapPlanetCard::AvailableTopSpace() const
{
	const double height = Height();
	return min(height, max(0., (number + 1) * height - parent->GetScroll()));
}



double MapPlanetCard::AvailableBottomSpace() const
{
	const Interface *mapInterface = GameData::Interfaces().Get("map detail panel");
	double maxPlanetPanelHeight = mapInterface->GetValue("max planet panel height");

	return min(Height(), max(0., Screen::Top() +
		min(MapDetailPanel::PlanetPanelHeight(), maxPlanetPanelHeight) - yCoordinate));
}
