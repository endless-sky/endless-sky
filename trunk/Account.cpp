/* Account.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Account.h"

#include "DataNode.h"
#include "Ship.h"

#include <sstream>

using namespace std;

namespace {
	static const unsigned HISTORY = 100;
}



// Default constructor.
Account::Account()
	: credits(0), salariesOwed(0), creditScore(400)
{
}



// Load or save account data.
void Account::Load(const DataNode &node)
{
	credits = 0;
	salariesOwed = 0;
	creditScore = 400;
	history.clear();
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "credits" && child.Size() >= 2)
			credits = child.Value(1);
		else if(child.Token(0) == "salaries" && child.Size() >= 2)
			salariesOwed = child.Value(1);
		else if(child.Token(0) == "score" && child.Size() >= 2)
			creditScore = child.Value(1);
		else if(child.Token(0) == "mortgage")
		{
			mortgages.push_back(Mortgage(0, 0, 0));
			mortgages.back().Load(child);
		}
		else if(child.Token(0) == "history")
			for(const DataNode &grand : child)
				history.push_back(grand.Value(0));
	}
}



void Account::Save(ostream &out) const
{
	out << "account\n";
	out << "\tcredits " << credits << "\n";
	if(salariesOwed)
		out << "\tsalaries " << salariesOwed << "\n";
	out << "\tscore " << creditScore << "\n";
	out << "\thistory\n";
	for(int worth : history)
		out << "\t\t" << worth << "\n";
	
	for(const Mortgage &mortgage : mortgages)
		mortgage.Save(out);
	
}



// Get or change the player's credits.
int Account::Credits() const
{
	return credits;
}



void Account::AddCredits(int value)
{
	credits += value;
}



void Account::PayExtra(int mortgage, int amount)
{
	if(static_cast<unsigned>(mortgage) >= mortgages.size() || amount > credits
			|| amount > mortgages[mortgage].Principal())
		return;
	
	mortgages[mortgage].PayExtra(amount);
	credits -= amount;
	
	if(!mortgages[mortgage].Principal())
		mortgages.erase(mortgages.begin() + mortgage);
}



// Step forward one day, and return a string summarizing payments made.
string Account::Step(int assets, int salaries)
{
	ostringstream out;
	
	salariesOwed += salaries;
	bool hasDebts = !mortgages.empty() || salariesOwed;
	bool paid = true;
	
	int salariesPaid = salariesOwed;
	if(salariesOwed)
	{
		if(salariesOwed > credits)
		{
			salariesPaid = credits;
			salariesOwed -= credits;
			credits = 0;
			paid = false;
			out << "You could not pay all your crew salaries. ";
		}
		else
		{
			credits -= salariesOwed;
			salariesOwed = 0;
		}
	}
	
	int mortgagesPaid = 0;
	int finesPaid = 0;
	for(Mortgage &mortgage : mortgages)
	{
		int payment = mortgage.Payment();
		if(payment > credits)
		{
			if(paid)
				out << "You missed a mortgage payment. ";
			paid = false;
		}
		else
		{
			payment = mortgage.MakePayment();
			credits -= payment;
			if(mortgage.Type() == "Mortgage")
				mortgagesPaid += payment;
			else
				finesPaid += payment;
		}
		assets -= mortgage.Principal();
	}
	for(auto it = mortgages.begin(); it != mortgages.end(); )
	{
		if(!it->Principal())
			it = mortgages.erase(it);
		else
			++it;
	}
	
	// Keep track of your net worth over the last HISTORY days.
	if(history.size() > HISTORY)
		history.erase(history.begin());
	history.push_back(credits + assets);
	
	if(hasDebts)
	{
		creditScore += paid ? 1 : -5;
		creditScore = max(200, min(800, creditScore));
	}
	
	if(!(salariesPaid + mortgagesPaid + finesPaid))
		return out.str();
	
	out << "You paid ";
	
	// If you made payments of all three types, the punctuation needs to
	// include commas, so just handle that separately here.
	if(salariesPaid && mortgagesPaid && finesPaid)
		out << salariesPaid << " credits in crew salaries, " << mortgagesPaid
			<< " in mortgages, and " << finesPaid << " in fines.";
	else
	{
		if(salariesPaid)
			out << salariesPaid << ((mortgagesPaid || finesPaid) ?
				" credits in crew salaries and " : " credits in crew salaries.");
		if(mortgagesPaid)
			out << mortgagesPaid << (salariesPaid ? " " : " credits ")
				<< (finesPaid ? "in mortgage payments and " : "in mortgage payments.");
		if(finesPaid)
			out << finesPaid << ((salariesPaid || mortgagesPaid) ?
				" in fines." : " credits in fines.");
	}
	return out.str();
}



// Liabilities:
const vector<Mortgage> &Account::Mortgages() const
{
	return mortgages;
}



void Account::AddMortgage(int principal)
{
	mortgages.emplace_back(principal, creditScore);
	credits += principal;
}



void Account::AddFine(int amount)
{
	mortgages.emplace_back(amount, 0, 60);
}



int Account::Prequalify() const
{
	return Mortgage::Maximum(YearlyRevenue(), creditScore);
}



// Assets:
int Account::NetWorth() const
{
	return history.back();
}



const vector<int> Account::History() const
{
	return history;
}



int Account::YearlyRevenue() const
{
	if(history.empty())
		return 0;
	
	return ((history.back() - history.front()) * 365) / HISTORY;
}



// Find out the player's credit rating.
int Account::CreditScore() const
{
	return creditScore;
}
