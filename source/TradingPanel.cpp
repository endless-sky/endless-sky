/* TradingPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TradingPanel.h"

#include "Color.h"
#include "Command.h"
#include "Dialog.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "System.h"
#include "UI.h"

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
	
	static const int MIN_X = -310;
	static const int MAX_X = 190;
	
	static const int NAME_X = -290;
	static const int PRICE_X = -150;
	static const int LEVEL_X = -110;
	static const int BUY_X = 0;
	static const int SELL_X = 60;
	static const int HOLD_X = 120;
	
	static const int FIRST_Y = 80;
}



TradingPanel::TradingPanel(PlayerInfo &player)
	: player(player), system(*player.GetSystem()), selectedRow(0)
{
	SetTrapAllEvents(false);
}



TradingPanel::~TradingPanel()
{
	if(profit)
	{
		string message = "You sold " + to_string(tonsSold)
			+ (tonsSold == 1 ? " ton" : " tons") + " of cargo ";
		
		if(profit < 0)
			message += "at a loss of " + Format::Number(-profit) + " credits.";
		else
			message += "for a total profit of " + Format::Number(profit) + " credits.";
		
		Messages::Add(message);
	}
}


	
void TradingPanel::Step()
{
	if(!Preferences::Has("help: trading"))
	{
		Preferences::Set("help: trading");
		GetUI()->Push(new Dialog(
			string("This is the trading panel. "
				"Earn money by buying commodities at a low price in one system, "
				"and selling at a higher price elsewhere. "
				"To view your map of commodity prices in other systems, press \"")
			+ Command::MAP.KeyName()
			+ string("\". To buy or sell, click on [buy] or [sell], "
				"or select a line with the up and down arrows and press \"+\" or \"-\" "
				"(or Enter and Delete).\n"
				"\tYou can buy 5 tons at once by holding down Shift, "
				"20 by holding down Control, "
				"or 100 at a time by holding down both.")));
	}
}



void TradingPanel::Draw()
{
	Color back = *GameData::Colors().Get("faint");
	FillShader::Fill(Point(-60., FIRST_Y + 20 * selectedRow + 33), Point(480., 20.), back);
	
	const Font &font = FontSet::Get(14);
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	
	int y = FIRST_Y;
	FillShader::Fill(Point(-60., y + 15.), Point(480., 1.), unselected);
	
	font.Draw("Commodity", Point(NAME_X, y), selected);
	font.Draw("Price", Point(PRICE_X, y), selected);
	
	string mod = "x " + to_string(Modifier());
	font.Draw(mod, Point(BUY_X, y), unselected);
	font.Draw(mod, Point(SELL_X, y), unselected);
	
	font.Draw("In Hold", Point(HOLD_X, y), selected);
	
	y += 5;
	int lastY = y + 20 * GameData::Commodities().size() + 25;
	font.Draw("free:", Point(SELL_X + 5, lastY), selected);
	font.Draw(to_string(player.Cargo().Free()), Point(HOLD_X, lastY), selected);
	
	int outfits = player.Cargo().OutfitsSize();
	int missionCargo = player.Cargo().MissionCargoSize();
	if(player.Cargo().HasOutfits() || missionCargo)
	{
		bool hasOutfits = false;
		bool hasHarvested = false;
		for(const auto &it : player.Cargo().Outfits())
			if(it.second)
			{
				bool isHarvested = (it.first->Get("installable") < 0.);
				(isHarvested ? hasHarvested : hasOutfits) = true;
			}
		
		string str = to_string(outfits + missionCargo);
		if(hasHarvested && missionCargo)
			str += " tons of mission cargo and other items.";
		else if(hasOutfits && missionCargo)
			str += " tons of outfits and mission cargo.";
		else if(hasOutfits && hasHarvested)
			str += " tons of outfits and harvested materials.";
		else if(hasOutfits)
			str += " tons of outfits.";
		else if(hasHarvested)
			str += " tons of harvested materials.";
		else
			str += " tons of mission cargo.";
		font.Draw(str, Point(NAME_X, lastY), unselected);
	}
	
	int i = 0;
	bool canSell = false;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		y += 20;
		int price = system.Trade(commodity.name);
		
		const Color &color = (i++ == selectedRow ? selected : unselected);
		font.Draw(commodity.name, Point(NAME_X, y), color);
		
		if(price)
		{
			font.Draw(to_string(price), Point(PRICE_X, y), color);
		
			int basis = player.GetBasis(commodity.name);
			if(basis && basis != price)
			{
				string profit = "(profit: " + to_string(price - basis) + ")";
				font.Draw(profit, Point(LEVEL_X, y), color);
			}
			else
			{
				int level = (price - commodity.low);
				if(level < 0)
					level = 0;
				else if(level >= (commodity.high - commodity.low))
					level = 4;
				else
					level = (5 * level) / (commodity.high - commodity.low);
				font.Draw(TRADE_LEVEL[level], Point(LEVEL_X, y), color);
			}
		
			font.Draw("[buy]", Point(BUY_X, y), color);
			font.Draw("[sell]", Point(SELL_X, y), color);
		}
		
		int hold = player.Cargo().Get(commodity.name);
		if(hold)
		{
			canSell |= (price != 0);
			font.Draw(to_string(hold), Point(HOLD_X, y), selected);
		}
	}
	
	const Interface *interface = GameData::Interfaces().Get("trade");
	Information info;
	if(player.Cargo().HasOutfits() || canSell)
		info.SetCondition("can sell");
	if(player.Cargo().Free() > 0)
		info.SetCondition("can buy");
	interface->Draw(info, this);
}



// Only override the ones you need; the default action is to return false.
bool TradingPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == SDLK_UP && selectedRow)
		--selectedRow;
	else if(key == SDLK_DOWN && selectedRow < static_cast<int>(GameData::Commodities().size()) - 1)
		++selectedRow;
	else if(key == '=' || key == SDLK_RETURN || key == SDLK_SPACE)
		Buy(1);
	else if(key == '-' || key == SDLK_BACKSPACE || key == SDLK_DELETE)
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
		for(const auto &it : player.Cargo().Outfits())
		{
			profit += it.second * it.first->Cost();
			tonsSold += it.second * static_cast<int>(it.first->Get("mass"));
			
			player.SoldOutfits()[it.first] += it.second;
			player.Accounts().AddCredits(it.second * it.first->Cost());
			player.Cargo().Remove(it.first, it.second);
		}
	}
	else if(command.Has(Command::MAP))
		GetUI()->Push(new MapDetailPanel(player, &selectedRow));
	else
		return false;
	
	return true;
}



bool TradingPanel::Click(int x, int y)
{
	int maxY = FIRST_Y + 25 + 20 * GameData::Commodities().size();
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		selectedRow = (y - FIRST_Y - 25) / 20;
		if(x >= BUY_X && x < SELL_X)
			Buy(1);
		else if(x >= SELL_X && x < HOLD_X)
			Buy(-1);
	}
	else
		return false;
	
	return true;
}



void TradingPanel::Buy(int64_t amount)
{
	amount *= Modifier();
	const string &type = GameData::Commodities()[selectedRow].name;
	int64_t price = system.Trade(type);
	if(!price)
		return;
	
	if(amount > 0)
	{
		amount = min(amount, player.Accounts().Credits() / price);
		amount = min(amount, static_cast<int64_t>(player.Cargo().Free()));
		player.AdjustBasis(type, amount * price);
	}
	else
	{
		// Selling cargo:
		amount = max(amount, static_cast<int64_t>(-player.Cargo().Get(type)));
		
		int64_t basis = player.GetBasis(type, amount);
		player.AdjustBasis(type, basis);
		profit += -amount * price + basis;
		tonsSold += -amount;
	}
	amount = player.Cargo().Transfer(type, -amount);
	player.Accounts().AddCredits(amount * price);
	GameData::AddPurchase(system, type, -amount);
}
