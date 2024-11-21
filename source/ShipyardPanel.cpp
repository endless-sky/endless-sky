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

#include "text/alignment.hpp"
#include "comparators/BySeriesAndIndex.h"
#include "ClickZone.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "FillShader.h"
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
#include "SpriteShader.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>

class System;

using namespace std;

namespace {
	// Label for the description field of the detail pane.
	const string DESCRIPTION = "description";
}



ShipyardPanel::ShipyardPanel(PlayerInfo &player)
	: ShopPanel(player, false), modifier(0)
{
	for(const auto &it : GameData::Ships())
		catalog[it.second.Attributes().Category()].push_back(it.first);

	for(pair<const string, vector<string>> &it : catalog)
		sort(it.second.begin(), it.second.end(), BySeriesAndIndex<Ship>());

	if(player.GetPlanet())
		shipyard = player.GetPlanet()->Shipyard();
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
	return BUTTON_HEIGHT + 40;
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
			const int swizzle = selectedShip->CustomSwizzle() >= 0
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



ShopPanel::TransactionResult ShipyardPanel::CanDoBuyButton () const
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

	GetUI()->Push(new ShipNameDialog(this, &ShipyardPanel::BuyShip, message));
}



void ShipyardPanel::Sell(bool storeOutfits)
{
	static const int MAX_LIST = 20;

	int count = playerShips.size();
	int initialCount = count;
	string message;

	if(storeOutfits && !planet->HasOutfitter())
	{
		message = "WARNING!\n\nThis planet has no Outfitter. "
			"There is no way to retain the outfits in storage.\n\n";
	}
	// Never allow keeping outfits where they cannot be retrieved.
	// TODO: Consider how to keep outfits in Cargo in the future.
	storeOutfits &= planet->HasOutfitter();

	if(!storeOutfits)
		message += "Sell the ";
	else if(count == 1)
		message += "Sell the hull of the ";
	else
		message = "Sell the hulls of the ";
	if(count == 1)
		message += playerShip->Name();
	else if(count <= MAX_LIST)
	{
		auto it = playerShips.begin();
		message += (*it++)->Name();
		--count;

		if(count == 1)
			message += " and ";
		else
		{
			while(count-- > 1)
				message += ",\n" + (*it++)->Name();
			message += ",\nand ";
		}
		message += (*it)->Name();
	}
	else
	{
		auto it = playerShips.begin();
		message += (*it++)->Name() + ",\n";
		for(int i = 1; i < MAX_LIST - 1; ++i)
			message += (*it++)->Name() + ",\n";

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



void ShipyardPanel::BuyShip(const string &name)
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



void ShipyardPanel::DrawButtons()
{
	// The last 70 pixels on the end of the side panel are for the buttons:
	Point buttonSize(SIDEBAR_WIDTH, ButtonPanelHeight());
	FillShader::Fill(Screen::BottomRight() - .5 * buttonSize, buttonSize,
		*GameData::Colors().Get("shop side panel background"));
	FillShader::Fill(
		Point(Screen::Right() - SIDEBAR_WIDTH / 2, Screen::Bottom() - ButtonPanelHeight()),
		Point(SIDEBAR_WIDTH, 1), *GameData::Colors().Get("shop side panel footer"));

	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	const Color &back = *GameData::Colors().Get("panel background");

	const Point creditsPoint(
		Screen::Right() - SIDEBAR_WIDTH + 10,
		Screen::Bottom() - 65);
	font.Draw("You have:", creditsPoint, dim);

	const auto credits = Format::CreditString(player.Accounts().Credits());
	font.Draw({ credits, {SIDEBAR_WIDTH - 20, Alignment::RIGHT} }, creditsPoint, bright);

	const Font &bigFont = FontSet::Get(18);
	const Color &hover = *GameData::Colors().Get("hover");
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");

	const Point buyCenter = Screen::BottomRight() - Point(210, 25);
	FillShader::Fill(buyCenter, Point(60, 30), back);
	const Color *buyTextColor = !CanDoBuyButton() ? &inactive : hoverButton == 'b' ? &hover : &active;
	string BUY = "_Buy";
	bigFont.Draw(BUY,
		buyCenter - .5 * Point(bigFont.Width(BUY), bigFont.Height()),
		*buyTextColor);

	const Point sellCenter = Screen::BottomRight() - Point(130, 25);
	FillShader::Fill(sellCenter, Point(60, 30), back);
	static const string SELL = "_Sell";
	bigFont.Draw(SELL,
		sellCenter - .5 * Point(bigFont.Width(SELL), bigFont.Height()),
		playerShip ? hoverButton == 's' ? hover : active : inactive);

	// TODO: Add button for sell but retain outfits.

	const Point leaveCenter = Screen::BottomRight() - Point(45, 25);
	FillShader::Fill(leaveCenter, Point(70, 30), back);
	static const string LEAVE = "_Leave";
	bigFont.Draw(LEAVE,
		leaveCenter - .5 * Point(bigFont.Width(LEAVE), bigFont.Height()),
		hoverButton == 'l' ? hover : active);

	const Point findCenter = Screen::BottomRight() - Point(580, 20);
	const Sprite *findIcon =
		hoverButton == 'f' ? SpriteSet::Get("ui/find selected") : SpriteSet::Get("ui/find unselected");
	SpriteShader::Draw(findIcon, findCenter);
	static const string FIND = "_Find";

	int modifier = Modifier();
	if(modifier > 1)
	{
		string mod = "x " + to_string(modifier);
		int modWidth = font.Width(mod);
		font.Draw(mod, buyCenter + Point(-.5 * modWidth, 10.), dim);
	}

	// Draw the tooltip for your full number of credits.
	const Rectangle creditsBox = Rectangle::FromCorner(creditsPoint, Point(SIDEBAR_WIDTH - 20, 15));
	if(creditsBox.Contains(ShopPanel::hoverPoint))
		ShopPanel::hoverCount += ShopPanel::hoverCount < ShopPanel::HOVER_TIME;
	else if(ShopPanel::hoverCount)
		--ShopPanel::hoverCount;

	if(ShopPanel::hoverCount == ShopPanel::HOVER_TIME)
	{
		string text = Format::Number(player.Accounts().Credits()) + " credits";
		DrawTooltip(text, hoverPoint, dim, *GameData::Colors().Get("tooltip background"));
	}
}



// Check if the given point is within the button zone, and if so return the
// letter of the button (or ' ' if it's not on a button).
char ShipyardPanel::CheckButton(int x, int y)
{
	// Check the Find button.
	if(x > Screen::Right() - SIDEBAR_WIDTH - 342 && x < Screen::Right() - SIDEBAR_WIDTH - 316 &&
		y > Screen::Bottom() - 31 && y < Screen::Bottom() - 4)
		return 'f';

	if(x < Screen::Right() - SIDEBAR_WIDTH || y < Screen::Bottom() - ButtonPanelHeight())
		return '\0';

	if(y < Screen::Bottom() - 40 || y >= Screen::Bottom() - 10)
		return ' ';

	x -= Screen::Right() - SIDEBAR_WIDTH;
	if(x > 9 && x < 70)
		// Check if it's the _Buy button.
		return 'b';
	else if(x > 89 && x < 150)
		// Check if it's the _Sell button:
		return 's';
	else if(x > 169 && x < 240)
		// Check if it's the _Leave button.
		return 'l';

	return ' ';
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
