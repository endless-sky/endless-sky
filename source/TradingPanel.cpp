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
#include "Dialog.h"
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
#include "Planet.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

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
	const size_t SELL_OUTFITS_DISPLAY_LIMIT = 15;
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

	int missionCargo = player.Cargo().MissionCargoSize();
	double minableCargo = player.Cargo().MinablesSizePrecise();
	double outfitCargo = max(0., player.Cargo().OutfitsSizePrecise() - minableCargo);
	if(minableCargo || outfitCargo || missionCargo)
	{
		string str = Format::MassString(ceil(minableCargo + outfitCargo + missionCargo)) + " of ";
		if(minableCargo && missionCargo)
			str += "mission cargo and other items.";
		else if(outfitCargo && missionCargo)
			str += "outfits and mission cargo.";
		else if(outfitCargo && minableCargo)
			str += "outfits and special commodities.";
		else if(outfitCargo)
			str += "outfits.";
		else if(minableCargo)
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
			canSell |= (price != 0);
			font.Draw(to_string(hold), Point(MIN_X + HOLD_X, y), selected);
		}
	}

	if(showProfit)
		font.Draw("Profit", Point(MIN_X + PROFIT_X, FIRST_Y), selected);

	canSellOutfits = outfitCargo &&
		(player.GetPlanet()->HasOutfitter() || Preferences::Has("'Sell Outfits' without outfitter"));
	Information info;
	if(canSellOutfits)
		info.SetCondition("can sell outfits");
	if(minableCargo)
		info.SetCondition("can sell specials");
	if(canSell)
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
	else if(key == 'P' || (key == 'p' && (mod & KMOD_SHIFT)))
	{
		if(Preferences::Has("Confirm 'Sell Specials' button"))
			GetUI()->Push(new Dialog([this]() { SellOutfitsOrMinables(true); },
				OutfitSalesMessage(true, SELL_OUTFITS_DISPLAY_LIMIT),
				Truncate::NONE, true, false));
		else
			SellOutfitsOrMinables(true);
	}
	else if((key == 'L' || (key == 'l' && (mod & KMOD_SHIFT))) && canSellOutfits)
	{
		if(Preferences::Has("Confirm 'Sell Outfits' button"))
			GetUI()->Push(new Dialog([this]() { SellOutfitsOrMinables(false); },
				OutfitSalesMessage(false, SELL_OUTFITS_DISPLAY_LIMIT),
				Truncate::NONE, true, false));
		else
			SellOutfitsOrMinables(false);
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



void TradingPanel::SellOutfitsOrMinables(bool sellMinable)
{
	int day = player.GetDate().DaysSinceEpoch();
	for(const auto &it : player.Cargo().Outfits())
	{
		if(sellMinable != static_cast<bool>(it.first->Get("minable")))
			continue;
		int64_t value = player.FleetDepreciation().Value(it.first, day, it.second);
		profit += value;
		tonsSold += static_cast<int>(it.second * it.first->Mass());

		player.AddStock(it.first, it.second);
		player.Accounts().AddCredits(value);
		player.Cargo().Remove(it.first, it.second);
	}
}



string TradingPanel::OutfitSalesMessage(bool sellMinable, size_t displayLimit)
{
	struct OutfitInfo {
		string name;
		int64_t count, value;
	};
	vector<OutfitInfo> outfitValue;
	double tonsSold = 0;
	int profit = 0;
	int day = player.GetDate().DaysSinceEpoch();
	for(auto &it : player.Cargo().Outfits())
	{
		if(sellMinable != static_cast<bool>(it.first->Get("minable")))
			continue;
		if(!it.second)
			continue;
		int64_t value = player.FleetDepreciation().Value(it.first, day, it.second);
		profit += value;
		tonsSold += static_cast<int>(it.second * it.first->Mass());
		// Store a description of the count & item, followed by its value.
		outfitValue.emplace_back("", it.second, value);
		if(sellMinable)
			outfitValue.back().name = Format::CargoString(it.second, it.first->DisplayName());
		else if(it.second == 1)
			outfitValue.back().name = it.first->DisplayName();
		else
			outfitValue.back().name = Format::Number(it.second) + " " + it.first->PluralName();
	}
	if(outfitValue.size() == 1)
		return "Sell " + outfitValue[0].name + " for " + Format::CreditString(profit) + "?";
	ostringstream out;
	out
		<< "Sell "
		<< Format::CargoString(tonsSold, sellMinable ? "of special commodities" : "of outfits")
		<< " for " << Format::CreditString(profit) << '?' << endl;

	// Sort by decreasing value.
	sort(outfitValue.begin(), outfitValue.end(), [](const OutfitInfo &left, const OutfitInfo &right)
	{
		return right.value < left.value;
	});
	const size_t toDisplay = min<int>(displayLimit, outfitValue.size());
	for(size_t i = 0; i < toDisplay; i++)
		out << outfitValue[i].name << endl;
	if(outfitValue.size() > displayLimit)
	{
		int64_t value = 0;
		for(size_t i = displayLimit; i < outfitValue.size(); i++)
			value += outfitValue[i].value;
		out << "and " << Format::CreditString(value) << " more.";
	}
	return out.str();
}
