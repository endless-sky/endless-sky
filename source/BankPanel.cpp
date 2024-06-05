/* BankPanel.cpp
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

#include "BankPanel.h"

#include "text/alignment.hpp"
#include "Color.h"
#include "Command.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "text/Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <string>

using namespace std;

namespace {
	// Column headings.
	const string HEADING[6] = {"Type", "Principal", "Interest", "Term", "Payment", ""};
	// X coordinates of the columns of the table.
	const int COLUMN[5] = {20, 130, 210, 280, 330};
	const int EXTRA_X = 410;

	// Maximum number of rows of mortages, etc. to draw.
	const int MAX_ROWS = 8;
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
	DoHelp("bank");
}



// Draw the bank information.
void BankPanel::Draw()
{
	// Draw the "Pay All" button.
	const Interface *bankUi = GameData::Interfaces().Get("bank");
	const Rectangle box = bankUi->GetBox("content");
	const int MIN_X = box.Left();
	const int FIRST_Y = box.Top();
	const int MAX_X = box.Right();

	// Set up the table that will contain most of the information.
	Table table;
	for(auto x : COLUMN)
		table.AddColumn(MIN_X + x);
	// The last column is for the "pay extra" button.
	table.AddColumn(MAX_X - 20, {Alignment::RIGHT});
	table.SetHighlight(MIN_X + 10, MAX_X - 10);
	table.DrawAt(Point(0., FIRST_Y));

	// Use stock colors from the game data.
	const Color &back = *GameData::Colors().Get("faint");
	const Color &unselected = *GameData::Colors().Get("medium");
	const Color &selected = *GameData::Colors().Get("bright");

	// Draw the heading of the table.
	table.SetColor(selected);
	for(const string &heading : HEADING)
		table.Draw(heading);
	table.DrawGap(5);

	// Figure out the total payments and principal (other than salaries). This
	// is in case there are more mortgages than can be displayed.
	int64_t otherPrincipal = 0;
	int64_t otherPayment = 0;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
	{
		otherPrincipal += mortgage.Principal();
		otherPayment += mortgage.Payment();
	}
	int64_t totalPayment = otherPayment;

	// Check if salaries need to be drawn.
	int64_t salaries = player.Salaries();
	int64_t crewSalariesOwed = player.Accounts().CrewSalariesOwed();
	int64_t salariesIncome = player.Accounts().SalariesIncomeTotal();
	int64_t tributeIncome = player.GetTributeTotal();

	// Check if maintenance needs to be drawn.
	PlayerInfo::FleetBalance b = player.MaintenanceAndReturns();
	int64_t maintenanceDue = player.Accounts().MaintenanceDue();
	// Figure out how many rows of the display are for mortgages, and also check
	// whether multiple mortgages have to be combined into the last row.
	mortgageRows = MAX_ROWS - (salaries != 0 || crewSalariesOwed != 0) - (b.maintenanceCosts != 0 || maintenanceDue != 0)
		- (b.assetsReturns != 0 || salariesIncome != 0 || tributeIncome != 0);
	int mortgageCount = player.Accounts().Mortgages().size();
	mergedMortgages = (mortgageCount > mortgageRows);
	if(!mergedMortgages)
		mortgageRows = mortgageCount;

	// Keep track of what row of the table we are on.
	int row = 0;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
	{
		// Color this row depending on whether it is selected or not.
		if(row == selectedRow)
		{
			table.DrawHighlight(back);
			table.SetColor(selected);
		}
		else
			table.SetColor(unselected);

		// Check if this is the last row we have space to draw. If so, check if
		// it must include a combination of multiple mortgages.
		bool isLastRow = (row == mortgageRows - 1);
		if(isLastRow && mergedMortgages)
		{
			table.Draw("Other");
			table.Draw(Format::Credits(otherPrincipal));
			// Skip the interest and term, because this entry represents the
			// combination of several different mortages.
			table.Advance(2);
			table.Draw(Format::Credits(otherPayment));
		}
		else
		{
			table.Draw(mortgage.Type());
			table.Draw(Format::Credits(mortgage.Principal()));
			table.Draw(mortgage.Interest());
			table.Draw(mortgage.Term());
			table.Draw(Format::Credits(mortgage.Payment()));

			// Keep track of how much out of the total principal and payment has
			// not yet been included in one of the rows of the table.
			otherPrincipal -= mortgage.Principal();
			otherPayment -= mortgage.Payment();
		}
		table.Draw("[pay extra]");
		++row;

		// Bail out if this was the last row we had space to draw.
		if(isLastRow)
			break;
	}
	table.SetColor(unselected);
	// Draw the salaries, if necessary.
	if(salaries || crewSalariesOwed)
	{
		// Include salaries in the total daily payment.
		totalPayment += salaries;

		table.Draw("Crew Salaries");
		// Check whether the player owes back salaries.
		if(crewSalariesOwed)
		{
			table.Draw(Format::Credits(crewSalariesOwed));
			table.Draw("(overdue)");
			table.Advance(1);
		}
		else
			table.Advance(3);
		table.Draw(Format::Credits(salaries));
		table.Advance();
	}
	// Draw the maintenance costs, if necessary.
	if(b.maintenanceCosts || maintenanceDue)
	{
		totalPayment += b.maintenanceCosts;

		table.Draw("Maintenance");
		if(maintenanceDue)
		{
			table.Draw(Format::Credits(maintenanceDue));
			table.Draw("(overdue)");
			table.Advance(1);
		}
		else
			table.Advance(3);
		table.Draw(Format::Credits(b.maintenanceCosts));
		table.Advance();
	}
	if(salariesIncome || tributeIncome || b.assetsReturns)
	{
		// Your daily income offsets expenses.
		totalPayment -= salariesIncome + tributeIncome + b.assetsReturns;

		const Interface *bankUi = GameData::Interfaces().Get("bank");
		const vector<std::string> labels = bankUi->GetStrings("income");
		const auto incomeLayout = Layout(310, Truncate::BACK);
		table.DrawCustom({labels[(salariesIncome != 0) + 2 * (tributeIncome != 0) + 4 * (b.assetsReturns != 0)],
			incomeLayout});
		// For crew salaries, only the "payment" field needs to be shown.
		table.Advance(3);
		table.Draw(Format::Credits(-(salariesIncome + tributeIncome + b.assetsReturns)));
		table.Advance();
	}

	// Draw the total daily payment.
	table.Advance(3);
	table.Draw("total:", selected);
	table.Draw(Format::Credits(totalPayment), unselected);
	table.Advance();

	// Draw the credit score.
	table.DrawAt(Point(0., FIRST_Y + 210.));
	string credit = "Your credit score is " + to_string(player.Accounts().CreditScore()) + ".";
	const auto scoreLayout = Layout(460, Truncate::MIDDLE);
	table.DrawCustom({credit, scoreLayout});
	table.Advance(5);

	// Report whether the player qualifies for a new loan.
	string amount;
	if(!qualify)
		amount = "You do not qualify for further loans at this time.";
	else
		amount = "You qualify for a new loan of up to " + Format::CreditString(qualify) + ".";
	if(qualify && selectedRow >= mortgageRows)
		table.DrawHighlight(back);
	const auto amountLayout = Layout(380, Truncate::MIDDLE);
	table.DrawCustom({amount, amountLayout}, unselected);
	if(qualify)
	{
		table.Advance(4);
		table.Draw("[apply]", selected);
	}

	Information info;
	if((crewSalariesOwed || maintenanceDue) && player.Accounts().Credits() > 0)
		info.SetCondition("can pay");
	else
		for(const Mortgage &mortgage : player.Accounts().Mortgages())
			if(mortgage.Principal() <= player.Accounts().Credits())
				info.SetCondition("can pay");
	bankUi->Draw(info, this);
}



// Handle key presses, or clicks that the interface has mapped to a key press.
bool BankPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(command.Has(Command::HELP))
	{
		DoHelp("bank advanced", true);
		DoHelp("bank", true);
	}
	else if(key == SDLK_UP && selectedRow)
		--selectedRow;
	else if(key == SDLK_DOWN && selectedRow < mortgageRows)
		++selectedRow;
	else if(key == SDLK_RETURN && selectedRow < mortgageRows)
	{
		GetUI()->Push(new Dialog(this, &BankPanel::PayExtra,
			"Paying off part of this debt will reduce your daily payments and the "
			"interest that it costs you. How many extra credits will you pay?"));
		DoHelp("bank advanced");
	}
	else if(key == SDLK_RETURN && qualify)
	{
		GetUI()->Push(new Dialog(this, &BankPanel::NewMortgage,
			"Borrow how many credits?"));
		DoHelp("bank advanced");
	}
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
		player.Accounts().PaySalaries(player.Accounts().CrewSalariesOwed());
		player.Accounts().PayMaintenance(player.Accounts().MaintenanceDue());
		qualify = player.Accounts().Prequalify();
	}
	else
		return false;

	return true;
}



// Handle mouse clicks.
bool BankPanel::Click(int x, int y, int clicks)
{
	const Interface *bankUi = GameData::Interfaces().Get("bank");
	const Rectangle box = bankUi->GetBox("content");
	const int MIN_X = box.Left();
	const int FIRST_Y = box.Top();
	const int MAX_X = box.Right();

	// Check if the click was on one of the rows of the table that represents a
	// mortgage or other current debt you have.
	int maxY = FIRST_Y + 25 + 20 * mortgageRows;
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		selectedRow = (y - FIRST_Y - 25) / 20;
		if(x >= MIN_X + EXTRA_X)
			DoKey(SDLK_RETURN);
	}
	else if(x >= MIN_X + EXTRA_X - 10 && x <= MAX_X && y >= FIRST_Y + 230 && y <= FIRST_Y + 250)
	{
		// If the player clicks the "apply" button, check if you qualify.
		if(qualify)
		{
			selectedRow = mortgageRows;
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
	bool isOther = (selectedRow == mortgageRows - 1 && mergedMortgages);

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
