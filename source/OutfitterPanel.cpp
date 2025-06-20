/* OutfitterPanel.cpp
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

#include "OutfitterPanel.h"

#include "text/Alignment.h"
#include "comparators/BySeriesAndIndex.h"
#include "Color.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Hardpoint.h"
#include "Mission.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "text/Truncate.h"
#include "UI.h"

#include <algorithm>
#include <limits>
#include <memory>

using namespace std;

namespace {
	// Label for the description field of the detail pane.
	const string DESCRIPTION = "description";
	const string LICENSE = " License";

	constexpr int checkboxSpacing = 20;

	// Button size/placement info:
	constexpr double BUTTON_ROW_START_PAD = 4.;
	constexpr double BUTTON_ROW_PAD = 6.;
	constexpr double BUTTON_COL_PAD = 6.;
	constexpr double BUTTON_WIDTH = 75.;

	// Determine the refillable ammunition a particular ship consumes or stores.
	set<const Outfit *> GetRefillableAmmunition(const Ship &ship) noexcept
	{
		auto toRefill = set<const Outfit *>{};
		auto armed = set<const Outfit *>{};
		for(auto &&it : ship.Weapons())
			if(it.GetOutfit())
			{
				const Outfit *weapon = it.GetOutfit();
				armed.emplace(weapon);
				if(weapon->Ammo() && weapon->AmmoUsage() > 0)
					toRefill.emplace(weapon->Ammo());
			}

		// Carriers may be configured to supply ammunition for carried ships found
		// within the fleet. Since a particular ammunition outfit is not bound to
		// any particular weapon (i.e. one weapon may consume it, while another may
		// only require it be installed), we always want to restock these outfits.
		for(auto &&it : ship.Outfits())
		{
			const Outfit *outfit = it.first;
			if(outfit->Ammo() && !outfit->IsWeapon() && !armed.contains(outfit))
				toRefill.emplace(outfit->Ammo());
		}
		return toRefill;
	}



	bool IsLicense(const string &name)
	{
		return name.ends_with(LICENSE);
	}



	string LicenseRoot(const string &name)
	{
		return name.substr(0, name.length() - LICENSE.length());
	}



	string UninstallActionName(OutfitterPanel::UninstallAction action)
	{
		switch(action)
		{
		case OutfitterPanel::UninstallAction::Uninstall:
			return "uninstall";
		case OutfitterPanel::UninstallAction::Store:
			return "store";
		case OutfitterPanel::UninstallAction::Sell:
			return "sell";
		}

		throw "unreachable";
	}
}



OutfitterPanel::OutfitterPanel(PlayerInfo &player, Sale<Outfit> stock)
	: ShopPanel(player, true), outfitter(stock)
{
	for(const pair<const string, Outfit> &it : GameData::Outfits())
		catalog[it.second.Category()].push_back(it.first);

	for(pair<const string, vector<string>> &it : catalog)
		sort(it.second.begin(), it.second.end(), BySeriesAndIndex<Outfit>());

	for(auto &ship : player.Ships())
		if(ship->GetPlanet() == planet)
			++shipsHere;
}



void OutfitterPanel::Step()
{
	CheckRefill();
	ShopPanel::Step();
	ShopPanel::CheckForMissions(Mission::OUTFITTER);
	if(GetUI()->IsTop(this) && !checkedHelp)
		// Use short-circuiting to only display one of them at a time.
		// (The first valid condition encountered will make us skip the others.)
		if(DoHelp("outfitter") || DoHelp("cargo management") || DoHelp("uninstalling and storage")
				|| (shipsHere > 1 && DoHelp("outfitter with multiple ships")) || true)
			// Either a help message was freshly displayed, or all of them have already been seen.
			checkedHelp = true;
}



int OutfitterPanel::TileSize() const
{
	return OUTFIT_SIZE;
}



int OutfitterPanel::VisibilityCheckboxesSize() const
{
	// Each checkbox is 20px, 4 checkboxes in total is 80px.
	return 80;
}



bool OutfitterPanel::HasItem(const string &name) const
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	if(showForSale && (outfitter.Has(outfit) || player.Stock(outfit) > 0))
		return true;

	if(showCargo && player.Cargo().Get(outfit))
		return true;

	if(showStorage && player.Storage().Get(outfit))
		return true;

	if(showInstalled)
		for(const Ship *ship : playerShips)
			if(ship->OutfitCount(outfit))
				return true;

	if(showForSale && HasLicense(name))
		return true;

	return false;
}



void OutfitterPanel::DrawItem(const string &name, const Point &point)
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	zones.emplace_back(point, Point(OUTFIT_SIZE, OUTFIT_SIZE), outfit);
	if(point.Y() + OUTFIT_SIZE / 2 < Screen::Top() || point.Y() - OUTFIT_SIZE / 2 > Screen::Bottom())
		return;

	bool isSelected = (outfit == selectedOutfit);
	bool isOwned = playerShip && playerShip->OutfitCount(outfit);
	DrawOutfit(*outfit, point, isSelected, isOwned);

	// Check if this outfit is a "license".
	bool isLicense = IsLicense(name);
	int mapSize = outfit->Get("map");

	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &highlight = *GameData::Colors().Get("outfitter difference highlight");

	bool highlightDifferences = false;
	if(playerShip || isLicense || mapSize)
	{
		int minCount = numeric_limits<int>::max();
		int maxCount = 0;
		if(isLicense)
			minCount = maxCount = player.HasLicense(LicenseRoot(name));
		else if(mapSize)
		{
			bool mapMinables = outfit->Get("map minables");
			minCount = maxCount = player.HasMapped(mapSize, mapMinables);
		}
		else
		{
			highlightDifferences = true;
			string firstModelName;
			for(const Ship *ship : playerShips)
			{
				// Highlight differences in installed outfit counts only when all selected ships are of the same model.
				string modelName = ship->TrueModelName();
				if(firstModelName.empty())
					firstModelName = modelName;
				else
					highlightDifferences &= (modelName == firstModelName);
				int count = ship->OutfitCount(outfit);
				minCount = min(minCount, count);
				maxCount = max(maxCount, count);
			}
		}

		if(maxCount)
		{
			string label = "installed: " + to_string(minCount);
			Color color = bright;
			if(maxCount > minCount)
			{
				label += " - " + to_string(maxCount);
				if(highlightDifferences)
					color = highlight;
			}

			Point labelPos = point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 38);
			font.Draw(label, labelPos, color);
		}
	}

	// Don't show the "in stock" amount if the outfit has an unlimited stock.
	int stock = 0;
	if(!outfitter.Has(outfit))
		stock = max(0, player.Stock(outfit));
	int cargo = player.Cargo().Get(outfit);
	int storage = player.Storage().Get(outfit);

	string message;
	if(cargo && storage && stock)
		message = "cargo+stored: " + to_string(cargo + storage) + ", in stock: " + to_string(stock);
	else if(cargo && storage)
		message = "in cargo: " + to_string(cargo) + ", in storage: " + to_string(storage);
	else if(cargo && stock)
		message = "in cargo: " + to_string(cargo) + ", in stock: " + to_string(stock);
	else if(storage && stock)
		message = "in storage: " + to_string(storage) + ", in stock: " + to_string(stock);
	else if(cargo)
		message = "in cargo: " + to_string(cargo);
	else if(storage)
		message = "in storage: " + to_string(storage);
	else if(stock)
		message = "in stock: " + to_string(stock);
	else if(!outfitter.Has(outfit))
		message = "(not sold here)";
	if(!message.empty())
	{
		Point pos = point + Point(
			OUTFIT_SIZE / 2 - 20 - font.Width(message),
			OUTFIT_SIZE / 2 - 24);
		font.Draw(message, pos, bright);
	}
}



double OutfitterPanel::ButtonPanelHeight() const
{
	// The 60 is for padding the credit and cargo space information lines.
	return 60. + BUTTON_HEIGHT * 3 + BUTTON_ROW_PAD * 2;
}



double OutfitterPanel::DrawDetails(const Point &center)
{
	string selectedItem = "Nothing Selected";
	const Font &font = FontSet::Get(14);

	double heightOffset = 20.;

	if(selectedOutfit)
	{
		outfitInfo.Update(*selectedOutfit, player, static_cast<bool>(CanUninstall(UninstallAction::Sell)),
			collapsed.contains(DESCRIPTION));
		selectedItem = selectedOutfit->DisplayName();

		const Sprite *thumbnail = selectedOutfit->Thumbnail();
		const float tileSize = thumbnail
			? max(thumbnail->Height(), static_cast<float>(TileSize()))
			: static_cast<float>(TileSize());
		const Point thumbnailCenter(center.X(), center.Y() + 20 + static_cast<int>(tileSize / 2));
		const Point startPoint(center.X() - INFOBAR_WIDTH / 2 + 20, center.Y() + 20 + tileSize);

		const Sprite *background = SpriteSet::Get("ui/outfitter unselected");
		SpriteShader::Draw(background, thumbnailCenter);
		if(thumbnail)
			SpriteShader::Draw(thumbnail, thumbnailCenter);

		const bool hasDescription = outfitInfo.DescriptionHeight();

		double descriptionOffset = hasDescription ? 40. : 0.;

		if(hasDescription)
		{
			if(!collapsed.contains(DESCRIPTION))
			{
				descriptionOffset = outfitInfo.DescriptionHeight();
				outfitInfo.DrawDescription(startPoint);
			}
			else
			{
				const Color &dim = *GameData::Colors().Get("medium");
				font.Draw(DESCRIPTION, startPoint + Point(35., 12.), dim);
				const Sprite *collapsedArrow = SpriteSet::Get("ui/collapsed");
				SpriteShader::Draw(collapsedArrow, startPoint + Point(20., 20.));
			}

			// Calculate the ClickZone for the description and add it.
			const Point descriptionDimensions(INFOBAR_WIDTH, descriptionOffset);
			const Point descriptionCenter(center.X(), startPoint.Y() + descriptionOffset / 2);
			ClickZone<string> collapseDescription = ClickZone<string>(
				descriptionCenter, descriptionDimensions, DESCRIPTION);
			categoryZones.emplace_back(collapseDescription);
		}

		const Point requirementsPoint(startPoint.X(), startPoint.Y() + descriptionOffset);
		const Point attributesPoint(startPoint.X(), requirementsPoint.Y() + outfitInfo.RequirementsHeight());
		outfitInfo.DrawRequirements(requirementsPoint);
		outfitInfo.DrawAttributes(attributesPoint);

		heightOffset = attributesPoint.Y() + outfitInfo.AttributesHeight();
	}

	// Draw this string representing the selected item (if any), centered in the details side panel.
	const Color &bright = *GameData::Colors().Get("bright");
	Point selectedPoint(center.X() - INFOBAR_WIDTH / 2, center.Y());
	font.Draw({selectedItem, {INFOBAR_WIDTH, Alignment::CENTER, Truncate::MIDDLE}},
		selectedPoint, bright);

	return heightOffset;
}



bool OutfitterPanel::ShouldHighlight(const Ship *ship)
{
	if(!selectedOutfit)
		return false;

	// If we're hovering above a button that can modify ship outfits, highlight
	// the ship.
	if(hoverButton == 'b')
		return CanDoBuyButton() && ShipCanAdd(ship, selectedOutfit);
	if(hoverButton == 'i')
		return CanInstall() && ShipCanAdd(ship, selectedOutfit);
	if(hoverButton == 's')
		return CanUninstall(UninstallAction::Sell) && ShipCanRemove(ship, selectedOutfit);
	if(hoverButton == 'u')
		return CanUninstall(UninstallAction::Uninstall) && ShipCanRemove(ship, selectedOutfit);

	return false;
}



// Draw the display filter selection checkboxes in the lower left of the outfit panel.
void OutfitterPanel::DrawKey()
{
	const Sprite *back = SpriteSet::Get("ui/outfitter key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));

	const Font &font = FontSet::Get(14);
	Color color[2] = {*GameData::Colors().Get("medium"), *GameData::Colors().Get("bright")};
	const Sprite *box[2] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};

	Point pos = Screen::BottomLeft() + Point(10., -VisibilityCheckboxesSize() + checkboxSpacing / 2.);
	const Point off{10., -.5 * font.Height()};
	const Point checkboxSize{180., checkboxSpacing};
	const Point checkboxOffset{80., 0.};

	SpriteShader::Draw(box[showForSale], pos);
	font.Draw("Show outfits for sale", pos + off, color[showForSale]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this](){ ToggleForSale(); });

	pos.Y() += checkboxSpacing;
	SpriteShader::Draw(box[showInstalled], pos);
	// The text color will be "medium" when no ships are selected, regardless of checkmark state,
	// indicating that the selection is invalid (invalid context).
	font.Draw("Show outfits installed", pos + off, color[showInstalled && playerShip]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this]() { ToggleInstalled(); });

	pos.Y() += checkboxSpacing;
	SpriteShader::Draw(box[showCargo], pos);
	font.Draw("Show outfits in cargo", pos + off, color[showCargo]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this](){ ToggleCargo(); });

	pos.Y() += checkboxSpacing;
	SpriteShader::Draw(box[showStorage], pos);
	font.Draw("Show outfits in storage", pos + off, color[showStorage]);
	AddZone(Rectangle(pos + checkboxOffset, checkboxSize), [this](){ ToggleStorage(); });
}



// Check if the given point is within the button zone, and if so return the
// letter of the button (or ' ' if it's not on a button).
char OutfitterPanel::CheckButton(int x, int y)
{
	const double rowOffsetY = BUTTON_HEIGHT + BUTTON_ROW_PAD;
	const double rowBaseY = Screen::BottomRight().Y() - 3. * rowOffsetY - BUTTON_ROW_START_PAD;
	const double buttonOffsetX = BUTTON_WIDTH + BUTTON_COL_PAD;
	const double w = BUTTON_WIDTH / 2;
	const double buttonCenterX = Screen::Right() - SIDEBAR_WIDTH / 2;

	// Check the Find button.
	if(x > Screen::Right() - SIDEBAR_WIDTH - 342 && x < Screen::Right() - SIDEBAR_WIDTH - 316 &&
		y > Screen::Bottom() - 31 && y < Screen::Bottom() - 4)
		return 'f';

	if(x < Screen::Right() - SIDEBAR_WIDTH || y < Screen::Bottom() - ButtonPanelHeight())
		return '\0';

	// Row 1
	if(rowBaseY < y && y <= rowBaseY + BUTTON_HEIGHT)
	{
		// Check if it's the _Buy button.
		if(buttonCenterX + buttonOffsetX * -1 - w <= x && x < buttonCenterX + buttonOffsetX * -1 + w)
			return 'b';
		// Check if it's the _Install button.
		if(buttonCenterX + buttonOffsetX * 0 - w <= x && x < buttonCenterX + buttonOffsetX * 0 + w)
			return 'i';
		// Check if it's the _Cargo button.
		if(buttonCenterX + buttonOffsetX * 1 - w <= x && x < buttonCenterX + buttonOffsetX * 1 + w)
			return 'c';
	}

	// Row 2
	if(rowBaseY + rowOffsetY < y && y <= rowBaseY + rowOffsetY + BUTTON_HEIGHT)
	{
		// Check if it's the _Sell button:
		if(buttonCenterX + buttonOffsetX * -1 - w <= x && x < buttonCenterX + buttonOffsetX * -1 + w)
			return 's';
		// Check if it's the _Uninstall button.
		if(buttonCenterX + buttonOffsetX * 0 - w <= x && x < buttonCenterX + buttonOffsetX * 0 + w)
			return 'u';
		// Check if it's the Sto_re button.
		if(buttonCenterX + buttonOffsetX * 1 - w <= x && x < buttonCenterX + buttonOffsetX * 1 + w)
			return 'r';
	}

	// Row 3
	if(rowBaseY + rowOffsetY * 2 < y && y <= rowBaseY + rowOffsetY * 2 + BUTTON_HEIGHT)
	{
		// Check if it's the _Leave button.
		if(buttonCenterX + buttonOffsetX * 1 - w <= x && x < buttonCenterX + buttonOffsetX * 1 + w)
			return 'l';
	}

	return ' ';
}



void OutfitterPanel::DrawButtons()
{
	// There will be two rows of buttons:
	//  [ Buy  ] [  Install  ] [  Cargo  ]
	//  [ Sell ] [ Uninstall ] [ Storage ]
	//                         [  Leave  ]
	const double rowOffsetY = BUTTON_HEIGHT + BUTTON_ROW_PAD;
	const double rowBaseY = Screen::BottomRight().Y() - 2.5 * rowOffsetY - BUTTON_ROW_START_PAD;
	const double buttonOffsetX = BUTTON_WIDTH + BUTTON_COL_PAD;
	const double buttonCenterX = Screen::Right() - SIDEBAR_WIDTH / 2;
	const Point buttonSize{BUTTON_WIDTH, BUTTON_HEIGHT};

	// Draw the button panel (shop side panel footer).
	const Point buttonPanelSize(SIDEBAR_WIDTH, ButtonPanelHeight());
	FillShader::Fill(Screen::BottomRight() - .5 * buttonPanelSize, buttonPanelSize,
		*GameData::Colors().Get("shop side panel background"));
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH / 2, Screen::Bottom() - ButtonPanelHeight()),
		Point(SIDEBAR_WIDTH, 1), *GameData::Colors().Get("shop side panel footer"));

	// Set up font size and colors for the credits.
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("panel background");

	// Draw the row for credits display.
	const Point creditsPoint(
		Screen::Right() - SIDEBAR_WIDTH + 10,
		Screen::Bottom() - ButtonPanelHeight() + 5);
	font.Draw("You have:", creditsPoint, dim);
	const string &credits = Format::CreditString(player.Accounts().Credits());
	font.Draw({credits, {SIDEBAR_WIDTH - 20, Alignment::RIGHT}}, creditsPoint, bright);

	// Draw the row for Fleet Cargo Space free.
	const Point cargoPoint(
		Screen::Right() - SIDEBAR_WIDTH + 10,
		Screen::Bottom() - ButtonPanelHeight() + 25);
	font.Draw("Cargo Free:", cargoPoint, dim);
	string space = Format::Number(player.Cargo().Free()) + " / " + Format::Number(player.Cargo().Size());
	font.Draw({space, {SIDEBAR_WIDTH - 20, Alignment::RIGHT}}, cargoPoint, bright);

	// Define the button text colors.
	const Font &bigFont = FontSet::Get(18);
	const Color &hover = *GameData::Colors().Get("hover");
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");

	auto DrawButton = [&](const string &name, const Point &center, bool isActive, bool hovering)
	{
		// Draw the first row of buttons.
		const Color *textColor = !isActive ? &inactive : hovering ? &hover : &active;
		FillShader::Fill(center, buttonSize, back);
		bigFont.Draw(name, center - .5 * Point(bigFont.Width(name),
			bigFont.Height()), *textColor);
	};

	// Row 1
	DrawButton("_Buy", Point(buttonCenterX + buttonOffsetX * -1, rowBaseY + rowOffsetY * 0),
		static_cast<bool>(CanDoBuyButton()), hoverButton == 'b');
	DrawButton("_Install", Point(buttonCenterX + buttonOffsetX * 0, rowBaseY + rowOffsetY * 0),
		static_cast<bool>(CanInstall()), hoverButton == 'i');
	DrawButton("_Cargo", Point(buttonCenterX + buttonOffsetX * 1, rowBaseY + rowOffsetY * 0),
		(CanMoveToCargoFromStorage() || CanBuyToCargo()), hoverButton == 'c');
	// Row 2
	DrawButton("_Sell", Point(buttonCenterX + buttonOffsetX * -1, rowBaseY + rowOffsetY * 1),
		static_cast<bool>(CanUninstall(UninstallAction::Sell)), hoverButton == 's');
	DrawButton("_Uninstall", Point(buttonCenterX + buttonOffsetX * 0, rowBaseY + rowOffsetY * 1),
		static_cast<bool>(CanUninstall(UninstallAction::Uninstall)), hoverButton == 'u');
	DrawButton("Sto_re", Point(buttonCenterX + buttonOffsetX * 1, rowBaseY + rowOffsetY * 1),
		static_cast<bool>(CanUninstall(UninstallAction::Store)), hoverButton == 'r');
	// Row 3
	DrawButton("_Leave", Point(buttonCenterX + buttonOffsetX * 1, rowBaseY + rowOffsetY * 2),
		true, hoverButton == 'l');

	// Draw the Modifier hover text that appears below the buttons when a modifier
	// is being applied.
	int modifier = Modifier();
	if(modifier > 1)
	{
		string mod = "x " + to_string(modifier);
		int modWidth = font.Width(mod);
		for(int i = -1; i < 2; i++)
			font.Draw(mod, Point(buttonCenterX + buttonOffsetX * i, rowBaseY + rowOffsetY * 0)
			+ Point(-.5 * modWidth, 10.), dim);
		for(int i = -1; i < 2; i++)
			font.Draw(mod, Point(buttonCenterX + buttonOffsetX * i, rowBaseY + rowOffsetY * 1)
			+ Point(-.5 * modWidth, 10.), dim);
	}

	// Draw tooltips for the button being hovered over:
	string tooltip;
	tooltip = GameData::Tooltip(string("outfitter: ") + hoverButton);
	if(!tooltip.empty())
		// Note: there is an offset between the cursor and tooltips in this case so that other
		// buttons can be seen as the mouse moves around.
		DrawTooltip(tooltip, hoverPoint + Point(-40, -60), dim, *GameData::Colors().Get("tooltip background"));

	// Draw the tooltip for your full number of credits and free cargo space
	const Rectangle creditsBox = Rectangle::FromCorner(creditsPoint, Point(SIDEBAR_WIDTH - 20, 30));
	if(creditsBox.Contains(hoverPoint))
		ShopPanel::hoverCount += ShopPanel::hoverCount < ShopPanel::HOVER_TIME;
	else if(ShopPanel::hoverCount)
		--ShopPanel::hoverCount;

	if(ShopPanel::hoverCount == ShopPanel::HOVER_TIME)
	{
		tooltip = Format::Number(player.Accounts().Credits()) + " credits" + "\n" +
			Format::Number(player.Cargo().Free()) + " tons free out of " +
			Format::Number(player.Cargo().Size()) + " tons total capacity";
		DrawTooltip(tooltip, hoverPoint, dim, *GameData::Colors().Get("tooltip background"));
	}
}



int OutfitterPanel::FindItem(const string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	auto it = zones.begin();
	for(unsigned int i = 0; i < zones.size(); ++i, ++it)
	{
		const Outfit *outfit = it->GetOutfit();
		int index = Format::Search(outfit->DisplayName(), text);
		if(index >= 0 && index < bestIndex)
		{
			bestIndex = index;
			bestItem = i;
			if(!index)
				return i;
		}
	}
	return bestItem;
}



ShopPanel::TransactionResult OutfitterPanel::HandleShortcuts(char key)
{
	TransactionResult result = false;
	if(key == 'b')
	{
		// Buy and install up to <modifier> outfits for each selected ship.
		result = CanDoBuyButton();
		if(result)
			DoBuyButton();
	}
	else if(key == 's')
	{
		// Sell <modifier> of the selected outfit from each selected ship.
		result = CanUninstall(UninstallAction::Sell);
		if(result)
			Sell(false);
	}
	else if(key == 'r')
	{
		// Move <modifier> of the selected outfit to storage from either cargo (first) or else each
		// of the selected ships.
		result = CanUninstall(UninstallAction::Store);
		if(result)
			RetainInStorage();
	}
	else if(key == 'c')
	{
		// Either move up to <multiple> outfits into cargo from storage if any are in storage, or else buy up to
		// <modifier> outfits into cargo.
		// Note: If the outfit is not able to be moved from storage or bought into cargo, give an error based on the buy
		// condition.
		if(CanMoveToCargoFromStorage())
			MoveToCargoFromStorage();
		else
		{
			// Selected outfit cannot be moved from storage, try buying:
			result = CanBuyToCargo();
			if(result)
				BuyIntoCargo();
		}
	}
	else if(key == 'i')
	{
		// Install up to <modifier> outfits from already owned equipment into each selected ship.
		result = CanInstall();
		if(result)
			Install();
	}
	else if(key == 'u')
	{
		// Move <modifier> of the selected outfit to storage from each selected ship or from cargo.
		// Uninstall up to <multiple> outfits from each of the selected ships if any are available to uninstall, or
		// else move up to <multiple> outfits from cargo into storage.
		// Note: If the outfit is not able to be uninstalled or moved from cargo, give an error based on the
		// uninstall condition.
		result = CanUninstall(UninstallAction::Uninstall);
		if(result)
			Uninstall();
		else if(CanUninstall(UninstallAction::Store))
		{
			RetainInStorage();
			// don't raise the original result that got us into this branch
			result = true;
		}
	}

	return result;
}



// Returns true if this ship can install the selected outfit.
bool OutfitterPanel::ShipCanAdd(const Ship *ship, const Outfit *outfit)
{
	return (ship->Attributes().CanAdd(*outfit, 1) > 0);
}



// Returns true if this ship can uninstall the selected outfit.
bool OutfitterPanel::ShipCanRemove(const Ship *ship, const Outfit *outfit)
{
	if(!ship->OutfitCount(outfit))
		return false;

	// If this outfit requires ammo, check if we could sell it if we sold all
	// the ammo for it first.
	const Outfit *ammo = outfit->Ammo();
	if(ammo && ship->OutfitCount(ammo))
	{
		Outfit attributes = ship->Attributes();
		attributes.Add(*ammo, -ship->OutfitCount(ammo));
		return attributes.CanAdd(*outfit, -1);
	}

	// Ammo is not a factor, check whether this ship can uninstall this outfit.
	return ship->Attributes().CanAdd(*outfit, -1);
}



void OutfitterPanel::DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned)
{
	const Sprite *thumbnail = outfit.Thumbnail();
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/outfitter selected" : "ui/outfitter unselected");
	SpriteShader::Draw(back, center);
	SpriteShader::Draw(thumbnail, center);

	// Draw the outfit name.
	const string &name = outfit.DisplayName();
	const Font &font = FontSet::Get(14);
	Point offset(-.5 * OUTFIT_SIZE, -.5 * OUTFIT_SIZE + 10.);
	font.Draw({name, {OUTFIT_SIZE, Alignment::CENTER, Truncate::MIDDLE}},
		center + offset, Color((isSelected | isOwned) ? .8 : .5, 0.));
}



bool OutfitterPanel::HasLicense(const string &name) const
{
	return (IsLicense(name) && player.HasLicense(LicenseRoot(name)));
}



void OutfitterPanel::CheckRefill()
{
	if(checkedRefill)
		return;
	checkedRefill = true;

	int count = 0;
	map<const Outfit *, int> needed;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip ships in other systems and those that were unable to land in-system.
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;

		++count;
		auto toRefill = GetRefillableAmmunition(*ship);
		for(const Outfit *outfit : toRefill)
		{
			int amount = ship->Attributes().CanAdd(*outfit, numeric_limits<int>::max());
			if(amount > 0)
			{
				bool available = outfitter.Has(outfit) || player.Stock(outfit) > 0;
				available = available || player.Cargo().Get(outfit) || player.Storage().Get(outfit);
				if(available)
					needed[outfit] += amount;
			}
		}
	}

	int64_t cost = 0;
	for(auto &it : needed)
	{
		// Don't count cost of anything installed from cargo or storage.
		it.second = max(0, it.second - player.Cargo().Get(it.first) - player.Storage().Get(it.first));
		if(!outfitter.Has(it.first))
			it.second = min(it.second, max(0, player.Stock(it.first)));
		cost += player.StockDepreciation().Value(it.first, day, it.second);
	}
	if(!needed.empty() && cost < player.Accounts().Credits())
	{
		string message = "Do you want to reload all the ammunition for your ship";
		message += (count == 1) ? "?" : "s?";
		if(cost)
			message += " It will cost " + Format::CreditString(cost) + ".";
		GetUI()->Push(new Dialog(this, &OutfitterPanel::Refill, message));
	}
}



void OutfitterPanel::Refill()
{
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		// Skip ships in other systems and those that were unable to land in-system.
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;

		auto toRefill = GetRefillableAmmunition(*ship);
		for(const Outfit *outfit : toRefill)
		{
			int neededAmmo = ship->Attributes().CanAdd(*outfit, numeric_limits<int>::max());
			if(neededAmmo > 0)
			{
				// Fill first from any stockpiles in storage.
				const int fromStorage = player.Storage().Remove(outfit, neededAmmo);
				neededAmmo -= fromStorage;
				// Then from cargo.
				const int fromCargo = player.Cargo().Remove(outfit, neededAmmo);
				neededAmmo -= fromCargo;
				// Then, buy at reduced (or full) price.
				int available = outfitter.Has(outfit) ? neededAmmo : min<int>(neededAmmo, max<int>(0, player.Stock(outfit)));
				if(neededAmmo && available > 0)
				{
					int64_t price = player.StockDepreciation().Value(outfit, day, available);
					player.Accounts().AddCredits(-price);
					player.AddStock(outfit, -available);
				}
				ship->AddOutfit(outfit, available + fromStorage + fromCargo);
			}
		}
	}
}



// Determine which ships of the selected ships should be referenced in this
// iteration of Buy / Sell.
vector<Ship *> OutfitterPanel::GetShipsToOutfit(bool isBuy) const
{
	vector<Ship *> shipsToOutfit;
	int compareValue = isBuy ? numeric_limits<int>::max() : 0;
	int compareMod = 2 * isBuy - 1;
	for(Ship *ship : playerShips)
	{
		if((isBuy && !ShipCanAdd(ship, selectedOutfit))
				|| (!isBuy && !ShipCanRemove(ship, selectedOutfit)))
			continue;

		int count = ship->OutfitCount(selectedOutfit);
		if(compareMod * count < compareMod * compareValue)
		{
			shipsToOutfit.clear();
			compareValue = count;
		}
		if(count == compareValue)
			shipsToOutfit.push_back(ship);
	}

	return shipsToOutfit;
}



// Return true if the player is able to purchase the item from the outfitter or
// previously sold stock and can afford it and is properly licensed (availability,
// payment, licensing).
ShopPanel::TransactionResult OutfitterPanel::CanPurchase(bool checkSpecialItems) const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	// If outfit is not available in the Outfitter, respond that it can't be bought here.
	if(!(outfitter.Has(selectedOutfit) || player.Stock(selectedOutfit) > 0))
	{
		// The store doesn't have it.
		return "You cannot buy this outfit here. "
			"It is being shown in the list because you have one, "
			"but this " + planet->Noun() + " does not sell them.";
	}

	// Check special unique outfits, if you already have them.
	// Skip this if told to for sake of speed
	if(checkSpecialItems)
	{
		int mapSize = selectedOutfit->Get("map");
		bool mapMinables = selectedOutfit->Get("map minables");
		if(mapSize > 0 && player.HasMapped(mapSize, mapMinables))
			return "You have already mapped all the systems shown by this map, "
				"so there is no reason to buy another.";

		if(HasLicense(selectedOutfit->TrueName()))
			return "You already have one of these licenses, "
				"so there is no reason to buy another.";
	}

	// Determine what you will have to pay to buy this outfit.
	int64_t cost = player.StockDepreciation().Value(selectedOutfit, day);
	int64_t credits = player.Accounts().Credits();
	if(cost > credits)
		return "You don't have enough money to buy this outfit. You need a further " +
			Format::CreditString(cost - credits);

	// Add the cost to buy the required license.
	int64_t licenseCost = LicenseCost(selectedOutfit, false);
	if(cost + licenseCost > credits)
		return "You don't have enough money to buy this outfit because you also need to buy a "
			"license for it. You need a further " +
			Format::CreditString(cost + licenseCost - credits);

	// Check that the player has any necessary licenses.
	if(licenseCost < 0)
		return "You cannot buy this outfit, because it requires a license that you don't have.";

	// The outfit can be purchased (available in the outfitter, licensed and affordable).
	return true;
}



// Checking where it will go (can it fit in fleet cargo), not checking where it will
// be sourced from.
ShopPanel::TransactionResult OutfitterPanel::CanFitInCargo(bool returnReason) const
{
	// Check fleet cargo space vs mass.
	double mass = selectedOutfit->Mass();
	double freeCargo = player.Cargo().FreePrecise();
	if(!mass || freeCargo >= mass)
		return true;

	// Don't waste time making strings that won't get used. However, we do need
	// to make the string here while we have the values available.
	if(returnReason)
	{
		return "You cannot load this outfit into cargo, because it takes up "
			+ Format::CargoString(mass, "mass") + " and your fleet has "
			+ Format::CargoString(freeCargo, "cargo space") + " free.";
	}

	return false;
}



// Checking where it will go (whether it actually be installed into any selected ship),
// not checking where it will be sourced from.
ShopPanel::TransactionResult OutfitterPanel::CanBeInstalled() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(!playerShip)
		return "No ship selected.";

	// Collect relevant errors.
	vector<string> errors;

	// Find if any ship can install the outfit.
	for(const Ship *ship : playerShips)
		if(ShipCanAdd(ship, selectedOutfit))
			return true;

	// If no selected ship can install the outfit, report error based on playerShip.
	double outfitNeeded = -selectedOutfit->Get("outfit space");
	double outfitSpace = playerShip->Attributes().Get("outfit space");
	if(outfitNeeded > outfitSpace)
		errors.push_back("You cannot install this outfit, because it takes up "
			+ Format::CargoString(outfitNeeded, "outfit space") + ", and this ship has "
			+ Format::MassString(outfitSpace) + " free.");

	double weaponNeeded = -selectedOutfit->Get("weapon capacity");
	double weaponSpace = playerShip->Attributes().Get("weapon capacity");
	if(weaponNeeded > weaponSpace)
		errors.push_back("Only part of your ship's outfit capacity is usable for weapons. "
			"You cannot install this outfit, because it takes up "
			+ Format::CargoString(weaponNeeded, "weapon space") + ", and this ship has "
			+ Format::MassString(weaponSpace) + " free.");

	double engineNeeded = -selectedOutfit->Get("engine capacity");
	double engineSpace = playerShip->Attributes().Get("engine capacity");
	if(engineNeeded > engineSpace)
		errors.push_back("Only part of your ship's outfit capacity is usable for engines. "
			"You cannot install this outfit, because it takes up "
			+ Format::CargoString(engineNeeded, "engine space") + ", and this ship has "
			+ Format::MassString(engineSpace) + " free.");

	if(selectedOutfit->Category() == "Ammunition")
		errors.emplace_back(!playerShip->OutfitCount(selectedOutfit) ?
			"This outfit is ammunition for a weapon. "
			"You cannot install it without first installing the appropriate weapon."
			: "You already have the maximum amount of ammunition for this weapon. "
			"If you want to install more ammunition, you must first install another of these weapons.");

	int mountsNeeded = -selectedOutfit->Get("turret mounts");
	int mountsFree = playerShip->Attributes().Get("turret mounts");
	if(mountsNeeded && !mountsFree)
		errors.emplace_back("This weapon is designed to be installed on a turret mount, "
			"but your ship does not have any unused turret mounts available.");

	int gunsNeeded = -selectedOutfit->Get("gun ports");
	int gunsFree = playerShip->Attributes().Get("gun ports");
	if(gunsNeeded && !gunsFree)
		errors.emplace_back("This weapon is designed to be installed in a gun port, "
			"but your ship does not have any unused gun ports available.");

	if(selectedOutfit->Get("installable") < 0.)
		errors.emplace_back("This item is not an outfit that can be installed in a ship.");

	// For unhandled outfit requirements, show a catch-all error message.
	if(errors.empty())
		errors.emplace_back("You cannot install this outfit in your ship, "
			"because it would reduce one of your ship's attributes to a negative amount.");

	// Return the errors in the appropriate format.
	if(errors.empty())
		return true;

	if(errors.size() == 1)
		return errors[0];

	string errorMessage = "There are several reasons why you cannot buy this outfit:\n";
	for(const string &error : errors)
		errorMessage += "- " + error + "\n";

	return errorMessage;
}



// Used in the "sell", "uninstall", and "store" to storage contexts. Are we able to remove Outfits from
// the selected ships ("sell"/"uninstall") or from cargo ("store") to storage?
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanUninstall(UninstallAction action) const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(!playerShip)
		return "No ship selected.";

	if(action == UninstallAction::Sell)
	{
		// Can sell things in Cargo.
		if(player.Cargo().Get(selectedOutfit))
			return true;

		// Can sell things in Storage.
		if(player.Storage().Get(selectedOutfit))
			return true;
	}
	else if(action == UninstallAction::Store)
	{
		// If we have one in cargo, we'll move that one.
		if(player.Cargo().Get(selectedOutfit))
			return true;
	}

	if(selectedOutfit->Get("map"))
		return "You cannot " + UninstallActionName(action) + " maps. Once you buy one, it is yours permanently.";

	if(HasLicense(selectedOutfit->TrueName()))
		return "You cannot " + UninstallActionName(action) + " licenses. Once you obtain one, it is yours permanently.";

	for(const Ship *ship : playerShips)
		if(ShipCanRemove(ship, selectedOutfit))
			return true;

	// None of the selected ships have any of this outfit in a state where it could be sold.
	// Either none of the ships even have the outfit:
	bool hasOutfit = false;
	for(const Ship *ship : playerShips)
		if(ship->OutfitCount(selectedOutfit))
		{
			hasOutfit = true;
			break;
		}

	if(!hasOutfit)
		return "You don't have any of these outfits to " + UninstallActionName(action) + ".";

	// At least one of the outfit is installed on at least one of the selected ships.
	// Create a complete summary of reasons why none of this outfit were able to be <verb>'d:
	// Looping over each ship which has the selected outfit, identify the reasons why it cannot be
	// <verb>'d. Make a list of ship to errors to assemble into a string later on:
	vector<pair<string, vector<string>>> dependentOutfitErrors;
	for(const Ship *ship : playerShips)
		if(ship->OutfitCount(selectedOutfit))
		{
			vector<string> errorDetails;
			for(const pair<const char *, double> &it : selectedOutfit->Attributes())
				if(ship->Attributes().Get(it.first) < it.second)
				{
					for(const auto &sit : ship->Outfits())
					{
						if(sit.first->Get(it.first) < 0.)
							errorDetails.emplace_back(string("the \"") + sit.first->DisplayName() + "\" "
								"depends on this outfit and must be uninstalled first");
					}
					if(errorDetails.empty())
						errorDetails.emplace_back(
							string("\"") + it.first + "\" value would be reduced to less than zero");
				}
			if(!errorDetails.empty())
				dependentOutfitErrors.emplace_back(ship->Name(), std::move(errorDetails));
		}

	// Return the errors in the appropriate format.
	if(dependentOutfitErrors.empty())
		return true;

	string errorMessage = "You cannot " + UninstallActionName(action) + " this from " +
		(playerShips.size() > 1 ? "any of the selected ships" : "your ship") + " because:\n";
	int i = 1;
	for(const auto &[shipName, errors] : dependentOutfitErrors)
	{
		if(playerShips.size() > 1)
		{
			errorMessage += to_string(i++) + ". You cannot " + UninstallActionName(action) + " this outfit from \"";
			errorMessage += shipName + "\" because:\n";
		}
		for(const string &error : errors)
			errorMessage += "- " + error + "\n";
	}
	return errorMessage;
}



// Return true if the player is able to purchase the item from the outfitter or
// previously sold stock and can afford it and the player is properly licensed,
// that is: availability, payment, and licensing, along with wheter the outfit
// will fit in cargo.
ShopPanel::TransactionResult OutfitterPanel::CanBuyToCargo() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	// Special case: maps, cannot place in Cargo.
	if(selectedOutfit->Get("map"))
		return "You place maps in to cargo.";

	if(HasLicense(selectedOutfit->TrueName()))
		return "You cannot place licenses into cargo.";

	TransactionResult canPurchase = CanPurchase(false);
	if(!canPurchase)
		return canPurchase;
	return CanFitInCargo(true);
}



// Check if the outfit can be purchased (availability, payment, licensing) and
// if it can be installed on any of the selected ships.
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanDoBuyButton() const
{
	TransactionResult result = CanPurchase();
	return result ? CanBeInstalled() : result;
}



// Check if the outfit is available to install (from already owned stores) as well as
// if it can actually be installed on any of the selected ships.
// If not, return the reasons why not.
ShopPanel::TransactionResult OutfitterPanel::CanInstall() const
{
	if(!planet || !selectedOutfit)
		return "No outfit selected.";

	if(!player.Storage().Get(selectedOutfit) && !player.Cargo().Get(selectedOutfit))
		return "You don't have any of this outfit to install, use \"B\" to buy (and install) it.";

	// Then check if installation requirements are met by at least one selected ship.
	return CanBeInstalled();
}



// Buy up to <modifier> of the selected outfit and place them in fleet cargo.
void OutfitterPanel::BuyIntoCargo()
{
	// Buy the required license.
	int64_t licenseCost = LicenseCost(selectedOutfit, false);
	if(licenseCost)
	{
		player.Accounts().AddCredits(-licenseCost);
		for(const string &licenseName : selectedOutfit->Licenses())
			if(!player.HasLicense(licenseName))
				player.AddLicense(licenseName);
	}

	int howManyToBuy = Modifier();
	double mass = selectedOutfit->Mass();
	if(mass)
		howManyToBuy = min(howManyToBuy, static_cast<int>(player.Cargo().FreePrecise() / mass));

	// How much will it cost to buy all that we can fit?
	int64_t price = player.StockDepreciation().Value(selectedOutfit, day, howManyToBuy);

	// Adjust the number to buy to stay within the player's available credits.
	while(price > player.Accounts().Credits() && howManyToBuy)
	{
		howManyToBuy--;
		price = player.StockDepreciation().Value(selectedOutfit, day, howManyToBuy);
	}

	// Buy as many as can be paid for and will fit.
	player.Accounts().AddCredits(-price);
	player.AddStock(selectedOutfit, -howManyToBuy);
	player.Cargo().Add(selectedOutfit, howManyToBuy);
}



// Buy and install up to <modifier> outfits for each selected ship.
void OutfitterPanel::DoBuyButton()
{
	BuyFromShopAndInstall();
}



// Sell <modifier> outfits, sourcing from cargo (first) or storage (second) or else the
// selected ships.
// Note: When selling from cargo or storage, up to <modifier> outfit will be sold in total,
// but when selling from the selected ships, up to <modifier> outfit will be sold from each ship.
// Note: sellOutfits is not used in the OutfitterPanel context.
void OutfitterPanel::Sell(bool /* storeOutfits */)
{
	int modifier = Modifier();
	// Now figure out where to sell it from:
	// Sell from Cargo if we have any there:
	if(player.Cargo().Get(selectedOutfit))
		for(int i = 0; i < modifier && player.Cargo().Get(selectedOutfit); ++i)
		{
			// Do the sale from cargo.
			player.Cargo().Remove(selectedOutfit);
			int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(price);
			player.AddStock(selectedOutfit, 1);
		}
	// Otherwise, sell from Storage if there are any available.
	else if(player.Storage().Get(selectedOutfit))
		for(int i = 0; i < modifier && player.Storage().Get(selectedOutfit); ++i)
		{
			// Do the sale from storage.
			player.Storage().Remove(selectedOutfit);
			int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(price);
			player.AddStock(selectedOutfit, 1);
		}
	// And lastly, sell from the selected ships.
	// Get the ships that have the most of this outfit installed.
	else if(!GetShipsToOutfit().empty())
		for(int i = 0; i < modifier && CanUninstall(UninstallAction::Sell); ++i)
			Uninstall(true);
}



// Install from already owned stores: cargo or storage.
// Note: Up to <modifier> for each selected ship will be installed.
void OutfitterPanel::Install()
{
	int modifier = Modifier();
	for(int i = 0; i < modifier && CanBeInstalled(); ++i)
	{
		// Find the ships with the fewest number of these outfits.
		const vector<Ship *> shipsToOutfit = GetShipsToOutfit(true);
		for(Ship *ship : shipsToOutfit)
		{
			if(!CanBeInstalled())
				return;

			// Use cargo first:
			if(player.Cargo().Get(selectedOutfit))
				player.Cargo().Remove(selectedOutfit);

			// Use storage second:
			else if(player.Storage().Get(selectedOutfit))
				player.Storage().Remove(selectedOutfit);

			// None were found in cargo or storage, bail out.
			else
				return;

			// Install the selected outfit to this ship.
			ship->AddOutfit(selectedOutfit, 1);
			int required = selectedOutfit->Get("required crew");
			if(required && ship->Crew() + required <= static_cast<int>(ship->Attributes().Get("bunks")))
				ship->AddCrew(required);
			ship->Recharge();
		}
	}
}



// Check if the outfit is able to be moved to cargo from storage.
// If not, return the reasons why not.
bool OutfitterPanel::CanMoveToCargoFromStorage() const
{
	if(!planet || !selectedOutfit)
		// No outfit selected.
		return false;

	if(selectedOutfit->Get("map"))
		// You cannot move maps around. Once you buy one, it is yours permanently.
		return false;

	if(HasLicense(selectedOutfit->TrueName()))
		// You cannot move licenses around. Once you obtain one, it is yours permanently.
		return false;

	if(!player.Storage().Get(selectedOutfit))
		// You don't have any of these outfits in storage to move to your cargo hold.
		return false;

	// The return value must be a bool.
	return static_cast<bool>(CanFitInCargo());
}



// Move up to <modifier> of selected outfit to cargo from storage.
void OutfitterPanel::MoveToCargoFromStorage()
{
	int modifier = Modifier();
	for(int i = 0; i < modifier && player.Storage().Get(selectedOutfit) && CanFitInCargo(); ++i)
	{
		player.Cargo().Add(selectedOutfit);
		player.Storage().Remove(selectedOutfit);
	}
}



// Move up to <modifier> of the selected outfit from fleet cargo to storage, or,
// if none are available in cargo, uninstall up to <modifier> outfits from each of the
// selected ships and move them to storage.
void OutfitterPanel::RetainInStorage()
{
	int modifier = Modifier();
	// If possible, move up to <modifier> of the selected outfit from fleet cargo to storage.
	int cargoCount = player.Cargo().Get(selectedOutfit);
	if(cargoCount)
	{
		// Transfer <cargoCount> outfits from cargo to storage.
		int howManyToMove = min(cargoCount, modifier);
		player.Cargo().Remove(selectedOutfit, howManyToMove);
		player.Storage().Add(selectedOutfit, howManyToMove);
	}
	// Otherwise, uninstall and move up to <modifier> selected outfits to storage from each
	// selected ship:
	else
		for(int i = 0; i < modifier && CanUninstall(UninstallAction::Store); ++i)
			Uninstall(false);
}



// Buy and install the <modifier> outfits into each selected ship.
// By definition Buy is from Outfitter (only).
void OutfitterPanel::BuyFromShopAndInstall() const
{
	// Buy the required license.
	int64_t licenseCost = LicenseCost(selectedOutfit, false);
	if(licenseCost)
	{
		player.Accounts().AddCredits(-licenseCost);
		for(const string &licenseName : selectedOutfit->Licenses())
			if(!player.HasLicense(licenseName))
				player.AddLicense(licenseName);
	}

	// Special case: maps.
	int mapSize = selectedOutfit->Get("map");
	if(mapSize)
	{
		bool mapMinables = selectedOutfit->Get("map minables");
		player.Map(mapSize, mapMinables);
		player.Accounts().AddCredits(-selectedOutfit->Cost());
		return;
	}

	// Special case: licenses.
	if(IsLicense(selectedOutfit->TrueName()))
	{
		player.AddLicense(LicenseRoot(selectedOutfit->TrueName()));
		player.Accounts().AddCredits(-selectedOutfit->Cost());
		return;
	}

	int modifier = Modifier();
	for(int i = 0; i < modifier && CanDoBuyButton(); ++i)
	{
		// Find the ships with the fewest number of these outfits.
		const vector<Ship *> shipsToOutfit = GetShipsToOutfit(true);
		for(Ship *ship : shipsToOutfit)
		{
			if(!CanDoBuyButton())
				return;

			// Pay for it and remove it from available stock.
			int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(-price);
			player.AddStock(selectedOutfit, -1);

			// Install it on this ship.
			ship->AddOutfit(selectedOutfit, 1);
			int required = selectedOutfit->Get("required crew");
			if(required && ship->Crew() + required <= static_cast<int>(ship->Attributes().Get("bunks")))
				ship->AddCrew(required);
			ship->Recharge();
		}
	}
}



// Uninstall up to <modifier> outfits from each of the selected ships and move them to storage,
// or, if none are installed, move up to <modifier> of the selected outfit from fleet cargo to
// storage.
void OutfitterPanel::Uninstall()
{
	int modifier = Modifier();
	// If installed on ship
	if(CanUninstall(UninstallAction::Uninstall))
		for(int i = 0; i < modifier && CanUninstall(UninstallAction::Uninstall); ++i)
			Uninstall(false);
	// Otherwise, move up to <modifier> of the selected outfit from fleet cargo to storage.
	else
	{
		int cargoCount = player.Cargo().Get(selectedOutfit);
		if(cargoCount)
		{
			// Transfer <cargoCount> outfits from cargo to storage.
			int howManyToMove = min(cargoCount, modifier);
			player.Cargo().Remove(selectedOutfit, howManyToMove);
			player.Storage().Add(selectedOutfit, howManyToMove);
		}
	}
}



// Uninstall outfits from selected ships, redeem credits if it was a sale.
// This also handles ammo dependencies.
// Note: This will remove one outfit from each selected ship which has the outfit.
void OutfitterPanel::Uninstall(bool sell) const
{
	// Key:
	//   's' - Sell
	//		Uninstall and redeem credits, item goes to Stock if not usually available.
	//   'u' - Uninstall
	//		Uninstall from ship and keep the item in Storage.

	// Get the ships that have the most of this outfit installed.
	const vector<Ship *> shipsToOutfit = GetShipsToOutfit();
	// Note: to get here, we have already confirmed that every ship in the selection
	// has the outfit and it is able to be uninstalled in the first place.
	for(Ship *ship : shipsToOutfit)
	{
		// Uninstall the outfit.
		ship->AddOutfit(selectedOutfit, -1);
		if(selectedOutfit->Get("required crew"))
			ship->AddCrew(-selectedOutfit->Get("required crew"));
		ship->Recharge();

		// If the context is sale:
		if(sell)
		{
			// Do the sale.
			int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(price);
			player.AddStock(selectedOutfit, 1);
		}
		// If the context is uninstall, move the outfit into Storage
		else
			// Move to storage.
			player.Storage().Add(selectedOutfit, 1);

		// Since some outfits have ammo, remove any ammo that must be sold because there
		// aren't enough supporting slots for said ammo once this outfit is removed.
		const Outfit *ammo = selectedOutfit->Ammo();
		if(ammo && ship->OutfitCount(ammo))
		{
			// Determine how many of this ammo we must uninstall to also uninstall the launcher.
			int mustUninstall = 0;
			for(const pair<const char *, double> &it : ship->Attributes().Attributes())
				if(it.second < 0.)
					mustUninstall = max<int>(mustUninstall, it.second / ammo->Get(it.first));

			if(mustUninstall)
			{
				ship->AddOutfit(ammo, -mustUninstall);

				// If the context is sale:
				if(sell)
				{
					// Do the sale.
					int64_t price = player.FleetDepreciation().Value(ammo, day, mustUninstall);
					player.Accounts().AddCredits(price);
					player.AddStock(ammo, mustUninstall);
				}
				// If the context is uninstall, move the outfit's ammo to storage
				else
					// Move to storage.
					player.Storage().Add(ammo, mustUninstall);
			}
		}
	}
}



void OutfitterPanel::ToggleForSale()
{
	showForSale = !showForSale;

	CheckSelection();
	delayedAutoScroll = true;
}



void OutfitterPanel::ToggleInstalled()
{
	showInstalled = !showInstalled;

	CheckSelection();
	delayedAutoScroll = true;
}



void OutfitterPanel::ToggleStorage()
{
	showStorage = !showStorage;

	CheckSelection();
	delayedAutoScroll = true;
}



void OutfitterPanel::ToggleCargo()
{
	showCargo = !showCargo;

	CheckSelection();
	delayedAutoScroll = true;
}
