/* ShipyardPanel.cpp
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

#include "ShipyardPanel.h"

#include "text/Alignment.h"
#include "comparators/BySeriesAndIndex.h"
#include "ClickZone.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Mission.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipNameDialog.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "text/Truncate.h"
#include "UI.h"

#include <algorithm>

class System;

using namespace std;

namespace {
	// Label for the description field of the detail pane.
	const string DESCRIPTION = "description";
}



ShipyardPanel::ShipyardPanel(PlayerInfo &player, Sale<Ship> stock)
	: ShopPanel(player, false), modifier(0), shipyard(stock)
{
	for(const auto &it : GameData::Ships())
		catalog[it.second.Attributes().Category()].push_back(it.first);

	for(pair<const string, vector<string>> &it : catalog)
		sort(it.second.begin(), it.second.end(), BySeriesAndIndex<Ship>());
}



void ShipyardPanel::Step()
{
	ShopPanel::Step();
	ShopPanel::CheckForMissions(Mission::SHIPYARD);
	if(GetUI()->IsTop(this))
		DoHelp("shipyard");
}



int ShipyardPanel::TileSize() const
{
	return SHIP_SIZE;
}



bool ShipyardPanel::HasItem(const string &name) const
{
	const Ship *ship = GameData::Ships().Get(name);
	return shipyard.Has(ship);
}



void ShipyardPanel::DrawItem(const string &name, const Point &point)
{
	const Ship *ship = GameData::Ships().Get(name);
	zones.emplace_back(point, Point(SHIP_SIZE, SHIP_SIZE), ship);
	if(point.Y() + SHIP_SIZE / 2 < Screen::Top() || point.Y() - SHIP_SIZE / 2 > Screen::Bottom())
		return;

	DrawShip(*ship, point, ship == selectedShip);
}



double ShipyardPanel::ButtonPanelHeight() const
{
	// The 50 = (3 x 10 (pad) + 20 x 1 (text)) for the credit information line.
	return 50 + BUTTON_HEIGHT * 2 + BUTTON_ROW_PAD * 1;
}



double ShipyardPanel::DrawDetails(const Point &center)
{
	string selectedItem = "No Ship Selected";
	const Font &font = FontSet::Get(14);

	double heightOffset = 20.;

	if(selectedShip)
	{
		shipInfo.Update(*selectedShip, player, collapsed.contains(DESCRIPTION), true);
		selectedItem = selectedShip->DisplayModelName();

		const Point spriteCenter(center.X(), center.Y() + 20 + TileSize() / 2);
		const Point startPoint(center.X() - INFOBAR_WIDTH / 2 + 20, center.Y() + 20 + TileSize());
		const Sprite *background = SpriteSet::Get("ui/shipyard selected");
		SpriteShader::Draw(background, spriteCenter);

		const Sprite *shipSprite = selectedShip->GetSprite();
		if(shipSprite)
		{
			const float spriteScale = min(1.f, (INFOBAR_WIDTH - 60.f) / max(shipSprite->Width(), shipSprite->Height()));
			const Swizzle *swizzle = selectedShip->CustomSwizzle()
				? selectedShip->CustomSwizzle() : GameData::PlayerGovernment()->GetSwizzle();
			SpriteShader::Draw(shipSprite, spriteCenter, spriteScale, swizzle);
		}

		const bool hasDescription = shipInfo.DescriptionHeight();

		double descriptionOffset = hasDescription ? 40. : 0.;

		if(hasDescription)
		{
			if(!collapsed.contains(DESCRIPTION))
			{
				descriptionOffset = shipInfo.DescriptionHeight();
				shipInfo.DrawDescription(startPoint);
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
			const ClickZone<string> collapseDescription = ClickZone<string>(
				descriptionCenter, descriptionDimensions, DESCRIPTION);
			categoryZones.emplace_back(collapseDescription);
		}

		const Point attributesPoint(startPoint.X(), startPoint.Y() + descriptionOffset);
		const Point outfitsPoint(startPoint.X(), attributesPoint.Y() + shipInfo.AttributesHeight());
		shipInfo.DrawAttributes(attributesPoint);
		shipInfo.DrawOutfits(outfitsPoint);

		heightOffset = outfitsPoint.Y() + shipInfo.OutfitsHeight();
	}

	// Draw this string representing the selected ship (if any), centered in the details side panel.
	const Color &bright = *GameData::Colors().Get("bright");
	const Point selectedPoint(center.X() - INFOBAR_WIDTH / 2, center.Y());
	font.Draw({selectedItem, {INFOBAR_WIDTH, Alignment::CENTER, Truncate::MIDDLE}},
		selectedPoint, bright);

	return heightOffset;
}



void ShipyardPanel::DrawButtons()
{
	// There will be two rows of buttons:
	//  [    Buy    ] [    Sell    ] [ Sell Hull ]
	//                               [   Leave   ]
	const double rowOffsetY = BUTTON_HEIGHT + BUTTON_ROW_PAD;
	const double rowBaseY = Screen::BottomRight().Y() - rowOffsetY - .5 * BUTTON_HEIGHT - BUTTON_ROW_START_PAD;
	const double buttonOffsetX = BUTTON_WIDTH + BUTTON_COL_PAD;
	const double buttonCenterX = Screen::Right() - SIDEBAR_WIDTH / 2 - 1.;
	const Point buttonSize{BUTTON_WIDTH, BUTTON_HEIGHT};

	// Draw the button panel (shop side panel footer).
	const Point buttonPanelSize(SIDEBAR_WIDTH, ButtonPanelHeight());
	const Rectangle buttonsFooter(Screen::BottomRight() - .5 * buttonPanelSize, buttonPanelSize);
	FillShader::Fill(buttonsFooter, *GameData::Colors().Get("shop side panel background"));
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH / 2, Screen::Bottom() - ButtonPanelHeight()),
		Point(SIDEBAR_WIDTH, 1), *GameData::Colors().Get("shop side panel footer"));

	// Set up font size and colors for the credits.
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");

	// Draw the row for credits display.
	const Point creditsPoint(
		Screen::Right() - SIDEBAR_WIDTH + 10,
		Screen::Bottom() - ButtonPanelHeight() + 10);
	font.Draw("You have:", creditsPoint, dim);

	const string credits = Format::CreditString(player.Accounts().Credits());
	font.Draw({credits, {SIDEBAR_WIDTH - 20, Alignment::RIGHT}}, creditsPoint, bright);

	// Clear the buttonZones, they will be populated again as buttons are drawn.
	buttonZones.clear();

	// Row 1
	DrawButton("_Buy",
		Rectangle(Point(buttonCenterX + buttonOffsetX * -1, rowBaseY + rowOffsetY * 0), buttonSize),
		static_cast<bool>(CanDoBuyButton()), hoverButton == 'b', 'b');
	DrawButton("_Sell",
		Rectangle(Point(buttonCenterX + buttonOffsetX * 0, rowBaseY + rowOffsetY * 0), buttonSize),
		static_cast<bool>(playerShips.size()), hoverButton == 's', 's');
	DrawButton("Sell H_ull",
		Rectangle(Point(buttonCenterX + buttonOffsetX * 1, rowBaseY + rowOffsetY * 0), buttonSize),
		static_cast<bool>(playerShips.size()), hoverButton == 'r', 'r');
	// Row 2
	DrawButton("_Leave",
		Rectangle(Point(buttonCenterX + buttonOffsetX * 1, rowBaseY + rowOffsetY * 1), buttonSize),
		true, hoverButton == 'l', 'l');

	// Draw the Modifier hover text that appears below the buttons when a modifier
	// is being applied.
	int modifier = Modifier();
	if(modifier > 1)
	{
		string mod = "x " + to_string(modifier);
		int modWidth = font.Width(mod);
		font.Draw(mod, Point(buttonCenterX + buttonOffsetX * -1, rowBaseY + rowOffsetY * 0)
		+ Point(-.5 * modWidth, 10.), dim);
	}

	// Draw tooltips for the button being hovered over:
	string tooltip = GameData::Tooltip(string("shipyard: ") + hoverButton);
	if(!tooltip.empty())
		buttonsTooltip.IncrementCount();
	else
		buttonsTooltip.DecrementCount();

	if(buttonsTooltip.ShouldDraw())
	{
		buttonsTooltip.SetZone(buttonsFooter);
		buttonsTooltip.SetText(tooltip, true);
		buttonsTooltip.Draw();
	}

	// Draw the tooltip for your full number of credits.
	const Rectangle creditsBox = Rectangle::FromCorner(creditsPoint, Point(SIDEBAR_WIDTH - 20, 15));
	if(creditsBox.Contains(hoverPoint))
		creditsTooltip.IncrementCount();
	else
		creditsTooltip.DecrementCount();

	if(creditsTooltip.ShouldDraw())
	{
		creditsTooltip.SetZone(creditsBox);
		creditsTooltip.SetText(Format::CreditString(player.Accounts().Credits(), false), true);
		creditsTooltip.Draw();
	}
}



ShopPanel::TransactionResult ShipyardPanel::HandleShortcuts(SDL_Keycode key)
{
	TransactionResult result = false;
	if(key == 'b')
	{
		// Buy up to <modifier> ships.
		result = CanDoBuyButton();
		if(result)
			DoBuyButton();
	}
	else if(key == 's')
	{
		// Sell selected ships and outfits.
		if(playerShip)
			Sell(false);
	}
	else if(key == 'r' || key == 'u')
	{
		// Sell selected ships and move outfits to Storage.
		if(playerShip)
			Sell(true);
	}

	return result;
}



ShopPanel::TransactionResult ShipyardPanel::CanDoBuyButton() const
{
	if(!selectedShip)
		return false;

	int64_t cost = player.StockDepreciation().Value(*selectedShip, day);

	// Check that the player has any necessary licenses.
	int64_t licenseCost = LicenseCost(&selectedShip->Attributes());
	if(licenseCost < 0)
		return "Buying this ship requires a special license. "
			"You will probably need to complete some sort of mission to get one.";

	// Check if the player can't pay.
	cost += licenseCost;
	if(player.Accounts().Credits() < cost)
	{
		// Check if ships could be sold to pay for the new ship.
		for(const auto &it : player.Ships())
			cost -= player.FleetDepreciation().Value(*it, day);

		if(player.Accounts().Credits() >= cost)
		{
			string ship = (player.Ships().size() == 1) ? "your current ship" : "some of your ships";
			return "You do not have enough credits to buy this ship. "
				"If you want to buy it, you must sell " + ship + " first.";
		}

		// Check if the license cost is the tipping point.
		if(player.Accounts().Credits() >= cost - licenseCost)
			return "You do not have enough credits to buy this ship, "
				"because it will cost you an extra " + Format::Credits(licenseCost) +
				" credits to buy the necessary licenses. "
				"Consider checking if the bank will offer you a loan.";

		return "You do not have enough credits to buy this ship. "
				"Consider checking if the bank will offer you a loan.";
	}
	return true;
}



void ShipyardPanel::DoBuyButton()
{
	int64_t licenseCost = LicenseCost(&selectedShip->Attributes());
	if(licenseCost < 0)
		return;

	modifier = Modifier();
	string message;
	if(licenseCost)
		message = "Note: you will need to pay " + Format::CreditString(licenseCost)
			+ " for the licenses required to operate this ship, in addition to its cost."
			" If that is okay with you, go ahead and enter a name for your brand new ";
	else
		message = "Enter a name for your brand new ";

	if(modifier == 1)
		message += selectedShip->DisplayModelName() + "! (Or leave it blank to use a randomly chosen name.)";
	else
		message += selectedShip->PluralModelName() + "! (Or leave it blank to use randomly chosen names.)";

	GetUI()->Push(new ShipNameDialog(this,
			Dialog::FunctionButton(this, "Buy", 'b', &ShipyardPanel::BuyShip),
			message));
}



void ShipyardPanel::Sell(bool storeOutfits)
{
	static const int MAX_LIST = 20;

	int count = playerShips.size();
	int initialCount = count;
	string message;

	if(storeOutfits && !planet->HasOutfitter())
	{
		message = "WARNING: This planet has no Outfitter. "
			"There is no way to retain the outfits in storage.\n";
		storeOutfits = false;
	}
	// Never allow keeping outfits where they cannot be retrieved.
	// TODO: Consider how to keep outfits in Cargo in the future.

	if(!storeOutfits)
		message += "Sell the ";
	else if(count == 1)
		message = "Sell the hull of the ";
	else
		message = "Sell the hulls of the ";
	if(count == 1)
		message += playerShip->GivenName();
	else if(count <= MAX_LIST)
	{
		auto it = playerShips.begin();
		message += (*it++)->GivenName();
		--count;

		if(count == 1)
			message += " and ";
		else
		{
			while(count-- > 1)
				message += ",\n" + (*it++)->GivenName();
			message += ",\nand ";
		}
		message += (*it)->GivenName();
	}
	else
	{
		auto it = playerShips.begin();
		message += (*it++)->GivenName() + ",\n";
		for(int i = 1; i < MAX_LIST - 1; ++i)
			message += (*it++)->GivenName() + ",\n";

		message += "and " + to_string(count - (MAX_LIST - 1)) + " other ships";
	}
	// To allow calculating the sale price of all the ships in the list,
	// temporarily copy into a shared_ptr vector:
	vector<shared_ptr<Ship>> toSell;
	for(const auto &it : playerShips)
		toSell.push_back(it->shared_from_this());
	int64_t total = player.FleetDepreciation().Value(toSell, day, storeOutfits);

	message += ((initialCount > 2) ? "\nfor " : " for ") + Format::CreditString(total) + "?";

	if(storeOutfits)
	{
		message += " Any outfits will be placed in storage.";
		GetUI()->Push(new Dialog(this, &ShipyardPanel::SellShipChassis, message, Truncate::MIDDLE));
	}
	else
		GetUI()->Push(new Dialog(this, &ShipyardPanel::SellShipAndOutfits, message, Truncate::MIDDLE));
}



bool ShipyardPanel::BuyShip(const string &name)
{
	int64_t licenseCost = LicenseCost(&selectedShip->Attributes());
	if(licenseCost)
	{
		player.Accounts().AddCredits(-licenseCost);
		for(const string &licenseName : selectedShip->Attributes().Licenses())
			if(!player.HasLicense(licenseName))
				player.AddLicense(licenseName);
	}

	for(int i = 1; i <= modifier; ++i)
	{
		// If no name is given, choose a random name. Otherwise, if buying
		// multiple ships, append a number to the given ship name.
		string shipName = name;
		if(name.empty())
			shipName = GameData::Phrases().Get("civilian")->Get();
		else if(modifier > 1)
			shipName += " " + to_string(i);

		player.BuyShip(selectedShip, shipName);
	}

	playerShip = &*player.Ships().back();
	playerShips.clear();
	playerShips.insert(playerShip);
	CheckSelection();

	// Close the dialog.
	return true;
}



void ShipyardPanel::SellShipAndOutfits()
{
	SellShip(false);
}



void ShipyardPanel::SellShipChassis()
{
	SellShip(true);
}



void ShipyardPanel::SellShip(bool storeOutfits)
{
	for(Ship *ship : playerShips)
		player.SellShip(ship, storeOutfits);
	playerShips.clear();
	playerShip = nullptr;
	for(const shared_ptr<Ship> &ship : player.Ships())
		if(ship->GetSystem() == player.GetSystem() && !ship->IsDisabled())
		{
			playerShip = ship.get();
			break;
		}
	if(playerShip)
		playerShips.insert(playerShip);
}



int ShipyardPanel::FindItem(const string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	auto it = zones.begin();
	for(unsigned int i = 0; i < zones.size(); ++i, ++it)
	{
		const Ship *ship = it->GetShip();
		int index = Format::Search(ship->DisplayModelName(), text);
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
