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
#include "FillShader.h"
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

	const int NAME_X = 20;
	const int PRICE_X = 140;
	const int LEVEL_X = 180;
	const int PROFIT_X = 260;
	const int BUY_X = 310;
	const int SELL_X = 370;
	const int HOLD_X = 430;
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

		Messages::Add(message, Messages::Importance::High);
	}
}



void TradingPanel::Step()
{
	DoHelp("trading");
}



void TradingPanel::Draw()
{
	const Interface *tradeUi = GameData::Interfaces().Get("trade");
	const Rectangle box = tradeUi->GetBox("content");
	const int MIN_X = box.Left();
	const int FIRST_Y = box.Top();

	const Color &back = *GameData::Colors().Get("faint");
	int selectedRow = player.MapColoring();
	if(selectedRow >= 0 && selectedRow < COMMODITY_COUNT)
	{
		const Point center(MIN_X + box.Width() / 2, FIRST_Y + 20 * selectedRow + 33);
		const Point dimensions(box.Width() - 20., 20.);
		FillShader::Fill(center, dimensions, back);
	}

	const Font &font = FontSet::Get(14);
	const Color &unselected = *GameData::Colors().Get("medium");
	const Color &selected = *GameData::Colors().Get("bright");

	int y = FIRST_Y;
	font.Draw("Commodity", Point(MIN_X + NAME_X, y), selected);
	font.Draw("Price", Point(MIN_X + PRICE_X, y), selected);

	string mod = "x " + to_string(Modifier());
	font.Draw(mod, Point(MIN_X + BUY_X, y), unselected);
	font.Draw(mod, Point(MIN_X + SELL_X, y), unselected);

	font.Draw("In Hold", Point(MIN_X + HOLD_X, y), selected);

	y += 5;
	int lastY = y + 20 * COMMODITY_COUNT + 25;
	font.Draw("free:", Point(MIN_X + SELL_X + 5, lastY), selected);
	font.Draw(to_string(player.Cargo().Free()), Point(MIN_X + HOLD_X, lastY), selected);

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
		font.Draw(str, Point(MIN_X + NAME_X, lastY), unselected);
	}

	int i = 0;
	bool canSell = false;
	bool canBuy = false;
	bool showProfit = false;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		y += 20;
		int price = system.Trade(commodity.name);
		int hold = player.Cargo().Get(commodity.name);

		bool isSelected = (i++ == selectedRow);
		const Color &color = (isSelected ? selected : unselected);
		font.Draw(commodity.name, Point(MIN_X + NAME_X, y), color);

		if(price)
		{
			canBuy |= isSelected;
			font.Draw(to_string(price), Point(MIN_X + PRICE_X, y), color);

			int basis = player.GetBasis(commodity.name);
			if(basis && basis != price && hold)
			{
				string profit = to_string(price - basis);
				font.Draw(profit, Point(MIN_X + PROFIT_X, y), color);
				showProfit = true;
			}
			int level = (price - commodity.low);
			if(level < 0)
				level = 0;
			else if(level >= (commodity.high - commodity.low))
				level = 4;
			else
				level = (5 * level) / (commodity.high - commodity.low);
			font.Draw(TRADE_LEVEL[level], Point(MIN_X + LEVEL_X, y), color);

			font.Draw("[buy]", Point(MIN_X + BUY_X, y), color);
			font.Draw("[sell]", Point(MIN_X + SELL_X, y), color);
		}
		else
		{
			font.Draw("----", Point(MIN_X + PRICE_X, y), color);
			font.Draw("(not for sale)", Point(MIN_X + LEVEL_X, y), color);
		}

		if(hold)
		{
			sellOutfits = false;
			canSell |= (price != 0);
			font.Draw(to_string(hold), Point(MIN_X + HOLD_X, y), selected);
		}
	}

	if(showProfit)
		font.Draw("Profit", Point(MIN_X + PROFIT_X, FIRST_Y), selected);

	Information info;
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
	if(key == SDLK_UP)
		player.SetMapColoring(max(0, player.MapColoring() - 1));
	else if(key == SDLK_DOWN)
		player.SetMapColoring(max(0, min(COMMODITY_COUNT - 1, player.MapColoring() + 1)));
	else if(key == SDLK_EQUALS || key == SDLK_KP_PLUS || key == SDLK_PLUS || key == SDLK_RETURN || key == SDLK_SPACE)
		Buy(1);
	else if(key == SDLK_MINUS || key == SDLK_KP_MINUS || key == SDLK_BACKSPACE || key == SDLK_DELETE)
		Buy(-1);
	else if(key == 'B' || (key == 'b' && (mod & KMOD_SHIFT)))
		Buy(1000000000);
	else if(key == 'S' || (key == 's' && (mod & KMOD_SHIFT)))
	{
		for(const auto &it : GameData::Commodities())
		{
			int64_t amount = player.Cargo().Get(it.name);
			int64_t price = system.Trade(it.name);
			if(!price || !amount)
				continue;

			int64_t basis = player.GetBasis(it.name, -amount);
			player.AdjustBasis(it.name, basis);
			profit += amount * price + basis;
			tonsSold += amount;

			player.Cargo().Remove(it.name, amount);
			player.Accounts().AddCredits(amount * price);
			GameData::AddPurchase(system, it.name, -amount);
		}
		int day = player.GetDate().DaysSinceEpoch();
		for(const auto &it : player.Cargo().Outfits())
		{
			if(it.first->Get("minable") <= 0. && !sellOutfits)
				continue;

			int64_t value = player.FleetDepreciation().Value(it.first, day, it.second);
			profit += value;
			tonsSold += static_cast<int>(it.second * it.first->Mass());

			player.AddStock(it.first, it.second);
			player.Accounts().AddCredits(value);
			player.Cargo().Remove(it.first, it.second);
		}
	}
	else if(command.Has(Command::MAP))
		GetUI()->Push(new MapDetailPanel(player));
	else
		return false;

	return true;
}



bool TradingPanel::Click(int x, int y, int clicks)
{
	const Interface *tradeUi = GameData::Interfaces().Get("trade");
	const Rectangle box = tradeUi->GetBox("content");
	const int MIN_X = box.Left();
	const int FIRST_Y = box.Top();
	const int MAX_X = box.Right();
	int maxY = FIRST_Y + 25 + 20 * COMMODITY_COUNT;
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		player.SetMapColoring((y - FIRST_Y - 25) / 20);
		if(x >= MIN_X + BUY_X && x < MIN_X + SELL_X)
			Buy(1);
		else if(x >= MIN_X + SELL_X && x < MIN_X + HOLD_X)
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
