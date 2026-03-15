/* TradingPanel.cpp
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

#include "TradingPanel.h"

#include "Color.h"
#include "Command.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <string>

using namespace std;

namespace {
	const string TRADE_LEVEL[] = {
		"(very low)",
		"(low)",
		"(medium)",
		"(high)",
		"(very high)"
	};
}



TradingPanel::TradingPanel(PlayerInfo &player)
	: player(player), system(*player.GetSystem()), COMMODITY_COUNT(GameData::Commodities().size())
{
	SetTrapAllEvents(false);
}



TradingPanel::~TradingPanel()
{
	if(profit)
	{
		string message = "You sold " + Format::CargoString(tonsSold, "cargo ");

		if(profit < 0)
			message += "at a loss of " + Format::CreditString(-profit) + ".";
		else
			message += "for a total profit of " + Format::CreditString(profit) + ".";

		Messages::Add({message, GameData::MessageCategories().Get("normal")});
	}
}



void TradingPanel::Step()
{
	DoHelp("trading");
}



void TradingPanel::Draw()
{
	Information info;
	const Interface *tradeUi = GameData::Interfaces().Get(Screen::Width() < 1280 ? "trade (small screen)" : "trade");
	Rectangle box = tradeUi->GetBox("content");
	int minX = box.Left();
	int firstY = box.Top();
	int nameX = tradeUi->GetValue("column: name");
	int priceX = tradeUi->GetValue("column: price");
	int levelX = tradeUi->GetValue("column: price level");
	int profitX = tradeUi->GetValue("column: profit");
	int buyX = tradeUi->GetValue("column: buy");
	int sellX = tradeUi->GetValue("column: sell");
	int holdX = tradeUi->GetValue("column: in cargo hold");

	const Color &back = *GameData::Colors().Get("faint");
	int selectedRow = player.MapColoring();
	if(selectedRow >= 0 && selectedRow < COMMODITY_COUNT)
	{
		const Point center(box.Center().X(), firstY + 20 * selectedRow + 33);
		const Point dimensions(box.Width() - 20., 20.);
		FillShader::Fill(center, dimensions, back);
	}

	const Font &font = FontSet::Get(14);
	const Color &unselected = *GameData::Colors().Get("medium");
	const Color &selected = *GameData::Colors().Get("bright");

	string mod = "x " + to_string(Modifier());
	info.SetString("multiplier", mod);

	info.SetString("free cargo space", to_string(player.Cargo().Free()));

	int outfits = player.Cargo().OutfitsSize();
	int missionCargo = player.Cargo().MissionCargoSize();
	sellOutfits = false;
	if(player.Cargo().HasOutfits() || missionCargo)
	{
		bool hasOutfits = false;
		bool hasMinables = false;
		for(const auto &it : player.Cargo().Outfits())
			if(it.second)
			{
				bool isMinable = it.first->Get("minable");
				(isMinable ? hasMinables : hasOutfits) = true;
			}
		sellOutfits = (hasOutfits && !hasMinables);

		string str = Format::MassString(outfits + missionCargo) + " of ";
		if(hasMinables && missionCargo)
			str += "mission cargo and other items.";
		else if(hasOutfits && missionCargo)
			str += "outfits and mission cargo.";
		else if(hasOutfits && hasMinables)
			str += "outfits and special commodities.";
		else if(hasOutfits)
			str += "outfits.";
		else if(hasMinables)
			str += "special commodities.";
		else
			str += "mission cargo.";
		info.SetString("other cargo", str);
	}

	int i = 0;
	int y = firstY + 5;
	bool canSell = false;
	bool canBuy = false;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		y += 20;
		int price = system.Trade(commodity.name);
		int hold = player.Cargo().Get(commodity.name);

		bool isSelected = (i++ == selectedRow);
		const Color &color = (isSelected ? selected : unselected);
		font.Draw(commodity.name, Point(minX + nameX, y), color);

		if(price)
		{
			canBuy |= isSelected;
			font.Draw(to_string(price), Point(minX + priceX, y), color);

			int basis = player.GetBasis(commodity.name);
			if(basis && basis != price && hold)
			{
				string profit = to_string(price - basis);
				font.Draw(profit, Point(minX + profitX, y), color);
				info.SetCondition("has profit");
			}
			int level = (price - commodity.low);
			if(level < 0)
				level = 0;
			else if(level >= (commodity.high - commodity.low))
				level = 4;
			else
				level = (5 * level) / (commodity.high - commodity.low);
			font.Draw(TRADE_LEVEL[level], Point(minX + levelX, y), color);

			font.Draw("[buy]", Point(minX + buyX, y), color);
			font.Draw("[sell]", Point(minX + sellX, y), color);
		}
		else
		{
			font.Draw("----", Point(minX + priceX, y), color);
			font.Draw("(not for sale)", Point(minX + levelX, y), color);
		}

		if(hold)
		{
			sellOutfits = false;
			canSell |= (price != 0);
			font.Draw(to_string(hold), Point(minX + holdX, y), selected);
		}
	}

	if(sellOutfits)
		info.SetCondition("can sell outfits");
	else if(player.Cargo().HasOutfits() || canSell)
		info.SetCondition("can sell");
	if(player.Cargo().Free() > 0 && canBuy)
		info.SetCondition("can buy");

	tradeUi->Draw(info, this);
}



// Only override the ones you need; the default action is to return false.
bool TradingPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(command.Has(Command::HELP))
		DoHelp("trading", true);
	else if(key == SDLK_UP)
		player.SetMapColoring(max(0, player.MapColoring() - 1));
	else if(key == SDLK_DOWN)
		player.SetMapColoring(max(0, min(COMMODITY_COUNT - 1, player.MapColoring() + 1)));
	else if(key == SDLK_EQUALS || key == SDLK_KP_PLUS || key == SDLK_PLUS || key == SDLK_RETURN || key == SDLK_SPACE)
		Buy(1);
	else if(key == SDLK_MINUS || key == SDLK_KP_MINUS || key == SDLK_BACKSPACE || key == SDLK_DELETE)
		Buy(-1);
	else if(key == 'u' || key == 'B' || (key == 'b' && (mod & KMOD_SHIFT)))
		Buy(1000000000);
	else if(key == 'e' || key == 'S' || (key == 's' && (mod & KMOD_SHIFT)))
	{
		for(const auto &it : player.Cargo().Commodities())
		{
			const string &commodity = it.first;
			const int64_t &amount = it.second;
			int64_t price = system.Trade(commodity);
			if(!price || !amount)
				continue;

			int64_t basis = player.GetBasis(commodity, -amount);
			profit += amount * price + basis;
			tonsSold += amount;

			GameData::AddPurchase(system, commodity, -amount);
			player.AdjustBasis(commodity, basis);
			player.Accounts().AddCredits(amount * price);
			player.Cargo().Remove(commodity, amount);
		}
		int day = player.GetDate().DaysSinceEpoch();
		for(const auto &it : player.Cargo().Outfits())
		{
			const Outfit *const outfit = it.first;
			const int64_t &amount = it.second;
			if(outfit->Get("minable") <= 0. && !sellOutfits)
				continue;

			int64_t value = player.FleetDepreciation().Value(outfit, day, amount);
			profit += value;
			tonsSold += static_cast<int>(amount * outfit->Mass());

			player.AddStock(outfit, amount);
			player.Accounts().AddCredits(value);
			player.Cargo().Remove(outfit, amount);
		}
	}
	else if(command.Has(Command::MAP))
		GetUI().Push(new MapDetailPanel(player));
	else
		return false;

	return true;
}



bool TradingPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;

	const Interface *tradeUi = GameData::Interfaces().Get(Screen::Width() < 1280 ? "trade (small screen)" : "trade");
	Rectangle box = tradeUi->GetBox("content");
	int minX = box.Left();
	int firstY = box.Top();
	int maxX = box.Right();
	int buyX = tradeUi->GetValue("column: buy");
	int sellX = tradeUi->GetValue("column: sell");
	int holdX = tradeUi->GetValue("column: in cargo hold");
	int maxY = firstY + 25 + 20 * COMMODITY_COUNT;
	if(x >= minX && x <= maxX && y >= firstY + 25 && y < maxY)
	{
		player.SetMapColoring((y - firstY - 25) / 20);
		if(x >= minX + buyX && x < minX + sellX)
			Buy(1);
		else if(x >= minX + sellX && x < minX + holdX)
			Buy(-1);
	}
	else
		return false;

	return true;
}



void TradingPanel::Buy(int64_t amount)
{
	int selectedRow = player.MapColoring();
	if(selectedRow < 0 || selectedRow >= COMMODITY_COUNT)
		return;

	amount *= Modifier();
	const string &type = GameData::Commodities()[selectedRow].name;
	int64_t price = system.Trade(type);
	if(!price)
		return;

	if(amount > 0)
	{
		amount = min(amount, min<int64_t>(player.Cargo().Free(), player.Accounts().Credits() / price));
		player.AdjustBasis(type, amount * price);
	}
	else
	{
		// Selling cargo:
		amount = max<int64_t>(amount, -player.Cargo().Get(type));

		int64_t basis = player.GetBasis(type, amount);
		player.AdjustBasis(type, basis);
		profit += -amount * price + basis;
		tonsSold += -amount;
	}
	amount = player.Cargo().Add(type, amount);
	player.Accounts().AddCredits(-amount * price);
	GameData::AddPurchase(system, type, amount);
}
