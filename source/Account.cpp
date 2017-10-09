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
#include "DataWriter.h"
#include "Format.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {
	// For tracking the player's average income, store daily net worth over this
	// number of days.
	const unsigned HISTORY = 100;
}



// Default constructor.
Account::Account()
	: credits(0), salariesOwed(0), creditScore(400)
{
}



// Load account information from a data file (saved game or starting conditions).
void Account::Load(const DataNode &node)
{
	credits = 0;
	salariesOwed = 0;
	creditScore = 400;
	history.clear();
	mortgages.clear();
	
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



// Write account information to a saved game file.
void Account::Save(DataWriter &out) const
{
	out.Write("account");
	out.BeginChild();
	{
		out.Write("credits", credits);
		if(salariesOwed)
			out.Write("salaries", salariesOwed);
		out.Write("score", creditScore);
		
		out.Write("history");
		out.BeginChild();
		{
			for(int64_t worth : history)
				out.Write(worth);
		}
		out.EndChild();
		
		for(const Mortgage &mortgage : mortgages)
			mortgage.Save(out);
	}
	out.EndChild();
}



// How much the player currently has in the bank.
int64_t Account::Credits() const
{
	return credits;
}



// Give the player credits (or pass  negative number to subtract). If subtracting,
// the calling function needs to check that this will not result in negative credits.
void Account::AddCredits(int64_t value)
{
	credits += value;
}



// Pay down extra principal on a mortgage.
void Account::PayExtra(int mortgage, int64_t amount)
{
	if(static_cast<unsigned>(mortgage) >= mortgages.size() || amount > credits
			|| amount > mortgages[mortgage].Principal())
		return;
	
	mortgages[mortgage].PayExtra(amount);
	credits -= amount;
	
	// If this payment was for the entire remaining amount in the mortgage,
	// remove it from the list.
	if(!mortgages[mortgage].Principal())
		mortgages.erase(mortgages.begin() + mortgage);
}



// Step forward one day, and return a string summarizing payments made.
string Account::Step(int64_t assets, int64_t salaries)
{
	ostringstream out;
	
	// Keep track of what payments were made and whether any could not be made.
	salariesOwed += salaries;
	bool paid = true;
	
	// Crew salaries take highest priority.
	int64_t salariesPaid = salariesOwed;
	if(salariesOwed)
	{
		if(salariesOwed > credits)
		{
			// If you can't pay the full salary amount, still pay some of it and
			// remember how much back wages you owe to your crew.
			salariesPaid = max<int64_t>(credits, 0);
			salariesOwed -= salariesPaid;
			credits -= salariesPaid;
			paid = false;
			out << "You could not pay all your crew salaries. ";
		}
		else
		{
			credits -= salariesOwed;
			salariesOwed = 0;
		}
	}
	
	// Unlike salaries, each mortgage payment must either be made in its entirety,
	// or skipped completely (accruing interest and reducing your credit score).
	int64_t mortgagesPaid = 0;
	int64_t finesPaid = 0;
	for(Mortgage &mortgage : mortgages)
	{
		int64_t payment = mortgage.Payment();
		if(payment > credits)
		{
			mortgage.MissPayment();
			if(paid)
				out << "You missed a mortgage payment. ";
			paid = false;
		}
		else
		{
			payment = mortgage.MakePayment();
			credits -= payment;
			// For the status text, keep track of whether this is a mortgage or a fine.
			if(mortgage.Type() == "Mortgage")
				mortgagesPaid += payment;
			else
				finesPaid += payment;
		}
		assets -= mortgage.Principal();
	}
	// If any mortgage has been fully paid off, remove it from the list.
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
	history.push_back(credits + assets - salariesOwed);
	
	// If you failed to pay any debt, your credit score drops. Otherwise, even
	// if you have no debts, it increases. (Because, having no debts at all
	// makes you at least as credit-worthy as someone who pays debts on time.)
	creditScore = max(200, min(800, creditScore + (paid ? 1 : -5)));
	
	// If you didn't make any payments, no need to continue further.
	if(!(salariesPaid + mortgagesPaid + finesPaid))
		return out.str();
	
	out << "You paid ";
	
	// If you made payments of all three types, the punctuation needs to
	// include commas, so just handle that separately here.
	if(salariesPaid && mortgagesPaid && finesPaid)
		out << Format::Number(salariesPaid) << " credits in crew salaries, " << Format::Number(mortgagesPaid)
			<< " in mortgages, and " << Format::Number(finesPaid) << " in fines.";
	else
	{
		if(salariesPaid)
			out << Format::Number(salariesPaid) << ((mortgagesPaid || finesPaid) ?
				" credits in crew salaries and " : " credits in crew salaries.");
		if(mortgagesPaid)
			out << Format::Number(mortgagesPaid) << (salariesPaid ? " " : " credits ")
				<< (finesPaid ? "in mortgage payments and " : "in mortgage payments.");
		if(finesPaid)
			out << Format::Number(finesPaid) << ((salariesPaid || mortgagesPaid) ?
				" in fines." : " credits in fines.");
	}
	return out.str();
}



int64_t Account::SalariesOwed() const
{
	return salariesOwed;
}



void Account::PaySalaries(int64_t amount)
{
	amount = min(min(amount, salariesOwed), credits);
	credits -= amount;
	salariesOwed -= amount;
}



// Access the list of mortgages.
const vector<Mortgage> &Account::Mortgages() const
{
	return mortgages;
}



// Add a new mortgage for the given amount, with an interest rate determined by
// your credit score.
void Account::AddMortgage(int64_t principal)
{
	mortgages.emplace_back(principal, creditScore);
	credits += principal;
}



// Add a "fine" with a high, fixed interest rate and a short term.
void Account::AddFine(int64_t amount)
{
	mortgages.emplace_back(amount, 0, 60);
}



// Check how big a mortgage the player can afford to pay at their current income.
int64_t Account::Prequalify() const
{
	int64_t payments = 0;
	int64_t liabilities = 0;
	for(const Mortgage &mortgage : mortgages)
	{
		payments += mortgage.Payment();
		liabilities += mortgage.Principal();
	}
	
	// Put a limit on new debt that the player can take out, as a fraction of
	// their net worth, to avoid absurd mortgages being offered when the player
	// has just captured some very lucrative ships.
	return max<int64_t>(0, min(
		NetWorth() / 3 + 500000 - liabilities,
		Mortgage::Maximum(YearlyRevenue(), creditScore, payments)));
}



// Get the player's total net worth (counting all ships and all debts).
int64_t Account::NetWorth() const
{
	return history.empty() ? 0 : history.back();
}



// Find out the player's credit rating.
int Account::CreditScore() const
{
	return creditScore;
}



// Extrapolate from the player's current net worth history to determine how much
// their net worth is expected to change over the course of the next year.
int64_t Account::YearlyRevenue() const
{
	if(history.empty() || history.back() <= history.front())
		return 0;
	
	// Note that this intentionally under-estimates if the player has not yet
	// played for long enough to accumulate a full income history.
	return ((history.back() - history.front()) * 365) / HISTORY;
}
