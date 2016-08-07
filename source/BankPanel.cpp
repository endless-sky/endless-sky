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
	// Dimensions of the table.
	static const int MIN_X = -310;
	static const int MAX_X = 190;
	
	// Column headings.
	static const string HEADING[6] = {"Type", "Principal", "Interest", "Term", "Payment", ""};
	// X coordinates of the columns of the table.
	static const int COLUMN[5] = {-290, -180, -100, -30, 20};
	static const int EXTRA_X = 100;
	
	// Position of the first line of the table.
	static const int FIRST_Y = 78;
}



// Constructor.
BankPanel::BankPanel(PlayerInfo &player)
	: player(player), qualify(player.Accounts().Prequalify())
{
	// This panel should allow events it does not respond to to pass through to
	// the underlying PlanetPanel.
	SetTrapAllEvents(false);
}



// This is called each frame when the bank is active.
void BankPanel::Step()
{
	// If the user has not yet been shown the help message, display it.
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



// Draw the bank information.
void BankPanel::Draw()
{
	// Set up the table that will contain most of the information.
	Table table;
	for(int x : COLUMN)
		table.AddColumn(x, Table::LEFT);
	// The last column is for the "pay extra" button.
	table.AddColumn(MAX_X - 20, Table::RIGHT);
	table.SetHighlight(MIN_X + 10, MAX_X - 10);
	table.DrawAt(Point(0., FIRST_Y));
	
	// Use stock colors from the game data.
	Color back = *GameData::Colors().Get("faint");
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	
	// Draw the heading of the table.
	table.DrawUnderline(unselected);
	table.SetColor(selected);
	for(const string &heading : HEADING)
		table.Draw(heading);
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
	
	// Keep track of what row of the table we are on.
	rowCount = 0;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
	{
		// Color this row depending on whether it is selected or not.
		if(rowCount == selectedRow)
		{
			table.DrawHighlight(back);
			table.SetColor(selected);
		}
		else
			table.SetColor(unselected);
		
		// There is room for seven rows if including salaries, or 8 if not.
		if(rowCount == (6 + !salaries) && otherPrincipal != mortgage.Principal())
		{
			table.Draw("Other", unselected);
			table.Draw(otherPrincipal);
			// Skip the interest and term, because this entry represents the
			// combination of several different mortages.
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
			
			// Keep track of how much out of the total principal and payment has
			// not yet been included in one of the rows of the table.
			otherPrincipal -= mortgage.Principal();
			otherPayment -= mortgage.Payment();
		}
		table.Draw("[pay extra]");
		++rowCount;
		
		// Draw no more than 8 rows, counting the salaries row if any.
		if(rowCount == 7 + !salaries)
			break;
	}
	table.SetColor(unselected);
	// Draw the salaries, if necessary.
	if(salaries)
	{
		// Include salaries in the total daily payment.
		totalPayment += salaries;
		
		table.Draw("Crew Salaries", unselected);
		// For crew salaries, only the "payment" field needs to be shown.
		table.Advance(3);
		table.Draw(salaries);
		table.Advance();
	}
	
	// Draw the total daily payment.
	table.Advance(3);
	table.Draw("total:", selected);
	table.Draw(totalPayment, unselected);
	table.Advance();
	
	// Draw the credit score.
	table.DrawAt(Point(0., FIRST_Y + 210.));
	string credit = "Your credit score is " + to_string(player.Accounts().CreditScore()) + ".";
	table.Draw(credit);
	table.Advance(5);
	
	// Report whether the player qualifies for a new loan.
	string amount;
	if(!qualify)
		amount = "You do not qualify for further loans at this time.";
	else
		amount = "You qualify for a new loan of up to " + Format::Number(qualify) + " credits.";
	bool isSelected = (qualify && selectedRow == rowCount);
	if(isSelected)
		table.DrawHighlight(back);
	table.Draw(amount, unselected);
	if(qualify)
	{
		table.Advance(4);
		table.Draw("[apply]", selected);
	}
	
	// Draw the "Pay All" button.
	const Interface *interface = GameData::Interfaces().Get("bank");
	Information info;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
		if(mortgage.Principal() <= player.Accounts().Credits())
			info.SetCondition("can pay");
	interface->Draw(info, this);
}



// Handle key presses, or clicks that the interface has mapped to a key press.
bool BankPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == SDLK_UP && selectedRow)
		--selectedRow;
	else if(key == SDLK_DOWN && selectedRow < rowCount)
		++selectedRow;
	else if(key == SDLK_RETURN && selectedRow < rowCount)
		GetUI()->Push(new Dialog(this, &BankPanel::PayExtra,
			"Paying off part of this debt will reduce your daily payments and the "
			"interest that it costs you. How many extra credits will you pay?"));
	else if(key == SDLK_RETURN && qualify)
		GetUI()->Push(new Dialog(this, &BankPanel::NewMortgage,
			"Borrow how many credits?"));
	else if(key == 'a')
	{
		// Pay all mortgages, skipping any you cannot afford to pay entirely.
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



// Handle mouse clicks.
bool BankPanel::Click(int x, int y)
{
	// Check if the click was on one of the rows of the table that represents a
	// mortgage or other current debt you have.
	int maxY = FIRST_Y + 25 + 20 * rowCount;
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		selectedRow = (y - FIRST_Y - 25) / 20;
		if(x >= EXTRA_X)
			DoKey(SDLK_RETURN);
	}
	else if(x >= EXTRA_X - 10 && x <= MAX_X && y >= FIRST_Y + 230 && y <= FIRST_Y + 250)
	{
		// If the player clicks the "apply" button, check if you qualify.
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



// Apply an extra payment to a debt. (This is a dialog callback.)
void BankPanel::PayExtra(const string &str)
{
	int64_t amount = static_cast<int64_t>(Format::Parse(str));
	// Check if the selected row is the "Other" row, which is only the case if
	// you have more mortages than can be displayed.
	const vector<Mortgage> &mortgages = player.Accounts().Mortgages();
	bool isOther = (selectedRow == rowCount - 1 && mortgages.size() > static_cast<unsigned>(rowCount));

	// Pay the mortgage. If this is the "other" row, loop through all the
	// mortgages included in that row.
	do {
		// You cannot pay more than you have or more than the mortgage principal.
		int64_t payment = min(amount, min(player.Accounts().Credits(), mortgages[selectedRow].Principal()));
		if(payment < 1)
			break;
		
		// Make an extra payment.
		player.Accounts().PayExtra(selectedRow, payment);
		amount -= payment;
	} while(isOther && static_cast<unsigned>(selectedRow) < mortgages.size());
	
	// Recalculate how much the player can prequalify for.
	qualify = player.Accounts().Prequalify();
}



// Apply for a new mortgage. (This is a dialog callback.)
void BankPanel::NewMortgage(const string &str)
{
	int64_t amount = static_cast<int64_t>(Format::Parse(str));
	
	// Limit the amount to whatever you have qualified for.
	amount = min(amount, qualify);
	if(amount > 0)
		player.Accounts().AddMortgage(amount);
	
	qualify = player.Accounts().Prequalify();
}
