/* BankPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "BankPanel.h"

#include "Color.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Preferences.h"
#include "Table.h"
#include "UI.h"

#include <string>

using namespace std;

namespace {
	static const int MIN_X = -310;
	static const int MAX_X = 190;
	
	static const int TYPE_X = -290;
	static const int PRINCIPAL_X = -180;
	static const int INTEREST_X = -100;
	static const int TERM_X = -30;
	static const int PAYMENT_X = 20;
	static const int EXTRA_X = 100;
	
	static const int FIRST_Y = 78;
	
	static const string HEADING[6] = {"Type", "Principal", "Interest", "Term", "Payment", ""};
}



BankPanel::BankPanel(PlayerInfo &player)
	: player(player), qualify(player.Accounts().Prequalify()), selectedRow(0)
{
	SetTrapAllEvents(false);
}



void BankPanel::Step()
{
	if(!Preferences::Has("help: bank"))
	{
		Preferences::Set("help: bank");
		GetUI()->Push(new Dialog(
			"This is the bank. "
			"Here, you can apply for new mortgages, if your income and credit history allows it. "
			"The bank is also a good place to get an overview of your daily expenses: "
			"mortgage payments, crew salaries, etc.\n"
			"\tPaying off a mortgage early means you pay less interest to the bank, "
			"but it is sometimes wiser to instead use your money to buy a bigger ship "
			"which can earn you more income."));
	}
}



void BankPanel::Draw() const
{
	Table table;
	table.AddColumn(TYPE_X, Table::LEFT);
	table.AddColumn(PRINCIPAL_X, Table::LEFT);
	table.AddColumn(INTEREST_X, Table::LEFT);
	table.AddColumn(TERM_X, Table::LEFT);
	table.AddColumn(PAYMENT_X, Table::LEFT);
	table.AddColumn(170, Table::RIGHT);
	table.SetHighlight(-300, 180);
	table.DrawAt(Point(0., FIRST_Y));
	
	Color back = *GameData::Colors().Get("faint");
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	table.DrawUnderline(unselected);
	table.SetColor(selected);
	for(int i = 0; i < 6; ++i)
		table.Draw(HEADING[i]);
	table.DrawGap(5);
	
	// Figure out the total payments and principal (other than salaries). This
	// is in case there are more mortgages than can be displayed.
	int otherPrincipal = 0;
	int otherPayment = 0;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
	{
		otherPrincipal += mortgage.Principal();
		otherPayment += mortgage.Payment();
	}
	int totalPayment = otherPayment;
	
	// Check if salaries need to be drawn.
	int salaries = player.Salaries();
	
	int row = 0;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
	{
		if(row == selectedRow)
		{
			table.DrawHighlight(back);
			table.SetColor(selected);
		}
		else
			table.SetColor(unselected);
		
		// There is room for seven rows if including salaries, or 8 if not.
		if(row == (6 + !salaries) && otherPrincipal != mortgage.Principal())
		{
			table.Draw("Other", unselected);
			table.Draw(otherPrincipal);
			table.Advance(2);
			table.Draw(otherPayment);
		}
		else
		{
			table.Draw(mortgage.Type());
			table.Draw(mortgage.Principal());
			table.Draw(mortgage.Interest());
			table.Draw(mortgage.Term());
			table.Draw(mortgage.Payment());
			
			otherPrincipal -= mortgage.Principal();
			otherPayment -= mortgage.Payment();
		}
		table.Draw("[pay extra]");
		++row;
		
		// Draw no more than 8 rows, counting the salaries row if any.
		if(row == 7 + !salaries)
			break;
	}
	table.SetColor(unselected);
	// Draw the salaries, if necessary.
	if(salaries)
	{
		totalPayment += salaries;
		
		table.Draw("Crew Salaries", unselected);
		table.Advance(3);
		table.Draw(salaries);
		table.Advance();
	}
	
	table.Advance(3);
	table.Draw("total:", selected);
	table.Draw(totalPayment, unselected);
	table.Advance();
	
	table.DrawAt(Point(0., FIRST_Y + 210.));
	string credit = "Your credit score is " + to_string(player.Accounts().CreditScore()) + ".";
	table.Draw(credit);
	table.Advance(5);
	
	string amount;
	if(!qualify)
		amount = "You do not qualify for further loans at this time.";
	else
		amount = "You qualify for a new loan of up to " + Format::Number(qualify) + " credits.";
	bool isSelected = qualify && (selectedRow > (6 + !salaries)
		|| static_cast<unsigned>(selectedRow) >= player.Accounts().Mortgages().size());
	if(isSelected)
		table.DrawHighlight(back);
	table.Draw(amount, unselected);
	if(qualify)
	{
		table.Advance(4);
		table.Draw("[apply]", selected);
	}
	
	const Interface *interface = GameData::Interfaces().Get("bank");
	Information info;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
		if(mortgage.Principal() <= player.Accounts().Credits())
			info.SetCondition("can pay");
	interface->Draw(info);
}



// Only override the ones you need; the default action is to return false.
bool BankPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	int maxRow = player.Accounts().Mortgages().size() - !qualify;
	if(key == SDLK_UP && selectedRow)
		--selectedRow;
	else if(key == SDLK_DOWN && selectedRow < maxRow)
		++selectedRow;
	else if(key == SDLK_RETURN
			&& static_cast<unsigned>(selectedRow) < player.Accounts().Mortgages().size())
		GetUI()->Push(new Dialog(this, &BankPanel::PayExtra,
			"Paying off part of this debt will reduce your daily payments and the "
			"interest that it costs you. How many extra credits will you pay?"));
	else if(key == SDLK_RETURN && qualify)
		GetUI()->Push(new Dialog(this, &BankPanel::NewMortgage,
			"Borrow how many credits?"));
	else if(key == 'a')
	{
		unsigned i = 0;
		while(i < player.Accounts().Mortgages().size())
		{
			int64_t principal = player.Accounts().Mortgages()[i].Principal();
			if(principal <= player.Accounts().Credits())
				player.Accounts().PayExtra(i, principal);
			else
				++i;
		}
	}
	else
		return false;
	
	return true;
}



bool BankPanel::Click(int x, int y)
{
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("bank");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key)
			return DoKey(key);
	}
	
	int maxY = FIRST_Y + 25 + 20 * min(7 + static_cast<size_t>(!player.Salaries()),
		player.Accounts().Mortgages().size());
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		selectedRow = (y - FIRST_Y - 25) / 20;
		if(x >= EXTRA_X)
			DoKey(SDLK_RETURN);
	}
	else if(x >= EXTRA_X - 10 && x <= MAX_X && y >= FIRST_Y + 230 && y <= FIRST_Y + 250)
	{
		if(qualify)
		{
			selectedRow = player.Accounts().Mortgages().size();
			DoKey(SDLK_RETURN);
		}
	}
	else
		return false;
	
	return true;
}



void BankPanel::PayExtra(const string &str)
{
	int64_t amount = static_cast<int64_t>(Format::Parse(str));
	int64_t payment = 0;
	bool isOther = selectedRow == 6 + !player.Salaries();

	// Pay the mortgage, if it's from Other pay it again until done.
	// You cannot pay more than you have or more than the mortgage principal.
	do {
		payment = min(amount, min(player.Accounts().Credits(),
			player.Accounts().Mortgages()[selectedRow].Principal()));
		if (payment < 1)
			break;
	
		player.Accounts().PayExtra(selectedRow, payment);
		amount -= payment;

	} while (isOther && static_cast<unsigned>(selectedRow) <
			player.Accounts().Mortgages().size());
	
	qualify = player.Accounts().Prequalify();
}



void BankPanel::NewMortgage(const string &str)
{
	int64_t amount = static_cast<int64_t>(Format::Parse(str));
	
	// Limit the amount to whatever you have qualified for.
	amount = min(amount, qualify);
	
	if(amount > 0)
		player.Accounts().AddMortgage(amount);
	
	qualify = player.Accounts().Prequalify();
}
