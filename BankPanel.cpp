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
#include "CreditsPanel.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "UI.h"

#include <string>

using namespace std;

namespace {
	static const int MIN_X = -310;
	static const int MAX_X = 190;
	
	static const int TYPE_X = -290;
	static const int PRINCIPAL_X = -200;
	static const int INTEREST_X = -120;
	static const int TERM_X = -40;
	static const int PAYMENT_X = 20;
	static const int EXTRA_X = 100;
	
	static const int FIRST_Y = 80;
}



BankPanel::BankPanel(PlayerInfo &player)
	: player(player), qualify(player.Accounts().Prequalify()),
	selectedRow(0), amount(0)
{
	SetTrapAllEvents(false);
}


	
void BankPanel::Step(bool isActive)
{
	if(isActive && amount)
	{
		if(static_cast<unsigned>(selectedRow) >= player.Accounts().Mortgages().size())
			player.Accounts().AddMortgage(amount);
		else
			player.Accounts().PayExtra(selectedRow, amount);
		
		amount = 0;
	}
}



void BankPanel::Draw() const
{
	Color back(.1, .1, .1, .1);
	if(static_cast<unsigned>(selectedRow) >= player.Accounts().Mortgages().size())
		FillShader::Fill(
			Point(130., FIRST_Y + 238), Point(100., 20.), back.Get());
	else
		FillShader::Fill(
			Point(-60., FIRST_Y + 20 * selectedRow + 33), Point(480., 20.), back.Get());
	
	const Font &font = FontSet::Get(14);
	Color unselected(.5, .5, .5, 1.);
	Color selected(.8, .8, .8, 1.);
	
	int y = FIRST_Y;
	FillShader::Fill(Point(-60., y + 15.), Point(480., 1.), unselected.Get());
	
	font.Draw("Type", Point(TYPE_X, y), selected.Get());
	font.Draw("Principal", Point(PRINCIPAL_X, y), selected.Get());
	font.Draw("Interest", Point(INTEREST_X, y), selected.Get());
	font.Draw("Term", Point(TERM_X, y), selected.Get());
	font.Draw("Payment", Point(PAYMENT_X, y), selected.Get());
	y += 5.;
	
	int total = 0;
	int i = 0;
	for(const Mortgage &mortgage : player.Accounts().Mortgages())
	{
		const float *color = (i++ == selectedRow) ? selected.Get() : unselected.Get();
		y += 20.;
		font.Draw(mortgage.Type(), Point(TYPE_X, y), color);
		font.Draw(to_string(mortgage.Principal()), Point(PRINCIPAL_X, y), color);
		font.Draw(mortgage.Interest(), Point(INTEREST_X, y), color);
		font.Draw(to_string(mortgage.Term()), Point(TERM_X, y), color);
		int p = mortgage.Payment();
		total += p;
		font.Draw(to_string(p), Point(PAYMENT_X, y), color);
		font.Draw("[pay extra]", Point(EXTRA_X, y), color);
	}
	int salaries = player.Accounts().Salaries();
	if(salaries)
	{
		y += 20.;
		font.Draw("Crew Salaries", Point(TYPE_X, y), unselected.Get());
		font.Draw(to_string(salaries), Point(PAYMENT_X, y), unselected.Get());
		total += salaries;
	}
	y += 20.;
	font.Draw("total:", Point(TERM_X, y), selected.Get());
	font.Draw(to_string(total), Point(PAYMENT_X, y), unselected.Get());
	
	y = FIRST_Y + 210.;
	string credit = "Your credit score is "
		+ to_string(player.Accounts().CreditScore()) + ".";
	font.Draw(credit, Point(TYPE_X, y), unselected.Get());
	
	y += 20.;
	string amount;
	if(!qualify)
		amount = "You do not qualify for further loans at this time.";
	else
		amount = "You qualify for a new loan of up to "
			+ to_string(qualify) + " credits.";
	font.Draw(amount, Point(TYPE_X, y), unselected.Get());
	if(qualify)
		font.Draw("[apply]", Point(EXTRA_X, y), selected.Get());
}



// Only override the ones you need; the default action is to return false.
bool BankPanel::KeyDown(SDLKey key, SDLMod mod)
{
	int maxRow = player.Accounts().Mortgages().size() - !qualify;
	if(key == SDLK_UP && selectedRow)
		--selectedRow;
	else if(key == SDLK_DOWN && selectedRow < maxRow)
		++selectedRow;
	else if(key == SDLK_RETURN
			&& static_cast<unsigned>(selectedRow) < player.Accounts().Mortgages().size())
		GetUI()->Push(new CreditsPanel("Pay how many credits?", &amount, min(
			player.Accounts().Credits(),
			player.Accounts().Mortgages()[selectedRow].Principal())));
	else if(key == SDLK_RETURN && qualify)
		GetUI()->Push(new CreditsPanel("Borrow how many credits?", &amount, qualify));
	else
		return false;
	
	return true;
}



bool BankPanel::Click(int x, int y)
{
	int maxY = FIRST_Y + 25 + 20 * player.Accounts().Mortgages().size();
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		selectedRow = (y - FIRST_Y - 25) / 20;
		if(x >= EXTRA_X)
		{
			GetUI()->Push(new CreditsPanel("Pay how many credits?", &amount, min(
				player.Accounts().Credits(),
				player.Accounts().Mortgages()[selectedRow].Principal())));
		}
	}
	else if(x >= EXTRA_X - 10 && x <= MAX_X && y >= FIRST_Y + 230 && y <= FIRST_Y + 250)
	{
		if(qualify)
		{
			selectedRow = player.Accounts().Mortgages().size();
			KeyDown(SDLK_RETURN, KMOD_NONE);
		}
	}
	else
		return false;
	
	return true;
}
