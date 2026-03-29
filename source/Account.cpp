/* Account.cpp
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

#include "Account.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"

#include <algorithm>
#include <numeric>
#include <sstream>

using namespace std;

namespace {
	// For tracking the player's average income, store daily net worth over this
	// number of days.
	const unsigned HISTORY = 100;
}



// Load account information from a data file (saved game or starting conditions).
void Account::Load(const DataNode &node, bool clearFirst)
{
	if(clearFirst)
	{
		credits = 0;
		crewSalariesOwed = 0;
		maintenanceDue = 0;
		creditScore = 400;
		mortgages.clear();
		history.clear();
	}

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "credits" && hasValue)
			credits = child.Value(1);
		else if(key == "salaries income")
		{
			for(const DataNode &grand : child)
			{
				if(grand.Size() < 2)
					grand.PrintTrace("Skipping incomplete salary income:");
				else
					salariesIncome[grand.Token(0)] = grand.Value(1);
			}
		}
		else if(key == "salaries" && hasValue)
			crewSalariesOwed = child.Value(1);
		else if(key == "maintenance" && hasValue)
			maintenanceDue = child.Value(1);
		else if(key == "score" && hasValue)
			creditScore = child.Value(1);
		else if(key == "mortgage")
			mortgages.emplace_back(child);
		else if(key == "history")
			for(const DataNode &grand : child)
				history.push_back(grand.Value(0));
		else
			child.PrintTrace("Skipping unrecognized account item:");
	}
}



// Write account information to a saved game file.
void Account::Save(DataWriter &out) const
{
	out.Write("account");
	out.BeginChild();
	{
		out.Write("credits", credits);
		if(!salariesIncome.empty())
		{
			out.Write("salaries income");
			out.BeginChild();
			{
				for(const auto &income : salariesIncome)
					out.Write(income.first, income.second);
			}
			out.EndChild();
		}
		if(crewSalariesOwed)
			out.Write("salaries", crewSalariesOwed);
		if(maintenanceDue)
			out.Write("maintenance", maintenanceDue);
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
string Account::Step(int64_t assets, int64_t salaries, int64_t maintenance)
{
	ostringstream out;

	// Keep track of what payments were made and whether any could not be made.
	crewSalariesOwed += salaries;
	maintenanceDue += maintenance;
	bool missedPayment = false;

	// Crew salaries take highest priority.
	int64_t salariesPaid = crewSalariesOwed;
	if(crewSalariesOwed)
	{
		if(crewSalariesOwed > credits)
		{
			// If you can't pay the full salary amount, still pay some of it and
			// remember how much back wages you owe to your crew.
			salariesPaid = max<int64_t>(credits, 0);
			crewSalariesOwed -= salariesPaid;
			credits -= salariesPaid;
			missedPayment = true;
			out << "You could not pay all your crew salaries.";
		}
		else
		{
			credits -= crewSalariesOwed;
			crewSalariesOwed = 0;
		}
	}

	// Maintenance costs are dealt with after crew salaries given that they act similarly.
	int64_t maintenancePaid = maintenanceDue;
	if(maintenanceDue)
	{
		if(maintenanceDue > credits)
		{
			// Like with crew salaries, maintenance costs can be paid in part with
			// the unpaid costs being paid later.
			maintenancePaid = max<int64_t>(credits, 0);
			maintenanceDue -= maintenancePaid;
			credits -= maintenancePaid;
			if(!missedPayment)
				out << "You could not pay all your maintenance costs.";
			missedPayment = true;
		}
		else
		{
			credits -= maintenanceDue;
			maintenanceDue = 0;
		}
	}

	// Unlike salaries, each mortgage payment must either be made in its entirety,
	// or skipped completely (accruing interest and reducing your credit score).
	int64_t mortgagesPaid = 0;
	int64_t finesPaid = 0;
	int64_t debtPaid = 0;
	for(Mortgage &mortgage : mortgages)
	{
		int64_t payment = mortgage.Payment();
		if(payment > credits)
		{
			mortgage.MissPayment();
			if(!missedPayment)
				out << "You missed a mortgage payment.";
			missedPayment = true;
		}
		else
		{
			payment = mortgage.MakePayment();
			credits -= payment;
			// For the status text, keep track of whether this is a mortgage, fine, or debt.
			if(mortgage.Type() == "Mortgage")
				mortgagesPaid += payment;
			else if(mortgage.Type() == "Fine")
				finesPaid += payment;
			else
				debtPaid += payment;
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
	history.push_back(credits + assets - crewSalariesOwed - maintenanceDue);

	// If you failed to pay any debt, your credit score drops. Otherwise, even
	// if you have no debts, it increases. (Because, having no debts at all
	// makes you at least as credit-worthy as someone who pays debts on time.)
	creditScore = max(200, min(800, creditScore + (missedPayment ? -5 : 1)));

	// If you didn't make any payments, no need to continue further.
	if(!(salariesPaid + maintenancePaid + mortgagesPaid + finesPaid + debtPaid))
		return out.str();
	else if(missedPayment)
		out << " ";

	out << "You paid ";

	map<string, int64_t> typesPaid;
	if(salariesPaid)
		typesPaid["crew salaries"] = salariesPaid;
	if(maintenancePaid)
		typesPaid["maintenance"] = maintenancePaid;
	if(mortgagesPaid)
		typesPaid["mortgages"] = mortgagesPaid;
	if(finesPaid)
		typesPaid["fines"] = finesPaid;
	if(debtPaid)
		typesPaid["debt"] = debtPaid;

	out << Format::List<map, string, int64_t>(typesPaid,
		[](const pair<string, int64_t> &it)
		{
			return Format::CreditString(it.second) + " in " + it.first;
		});
	out << '.';
	return out.str();
}



const map<string, int64_t> &Account::SalariesIncome() const
{
	return salariesIncome;
}



int64_t Account::SalariesIncomeTotal() const
{
	return accumulate(
		salariesIncome.begin(),
		salariesIncome.end(),
		0,
		[](int64_t value, const map<string, int64_t>::value_type &salary)
		{
			return value + salary.second;
		}
	);
}



void Account::SetSalaryIncome(const string &name, int64_t amount)
{
	if(amount == 0)
		salariesIncome.erase(name);
	else
		salariesIncome[name] = amount;
}



int64_t Account::CrewSalariesOwed() const
{
	return crewSalariesOwed;
}



void Account::PaySalaries(int64_t amount)
{
	amount = min(min(amount, crewSalariesOwed), credits);
	credits -= amount;
	crewSalariesOwed -= amount;
}



int64_t Account::MaintenanceDue() const
{
	return maintenanceDue;
}



void Account::PayMaintenance(int64_t amount)
{
	amount = min(min(amount, maintenanceDue), credits);
	credits -= amount;
	maintenanceDue -= amount;
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
	mortgages.emplace_back("Mortgage", principal, creditScore);
	credits += principal;
}



// Add a "fine" with a high, fixed interest rate and a short term.
void Account::AddFine(int64_t amount)
{
	mortgages.emplace_back("Fine", amount, 0, 60);
}



// Add debt with the given interest rate and term. If no interest rate is
// given then the player's credit score is used to determine the interest rate.
void Account::AddDebt(int64_t amount, optional<double> interest, int term)
{
	if(interest)
		mortgages.emplace_back("Debt", amount, *interest, term);
	else
		mortgages.emplace_back("Debt", amount, creditScore, term);
}



// Check how big a mortgage the player can afford to pay at their current income.
int64_t Account::Prequalify() const
{
	double payments = 0.;
	int64_t liabilities = 0;
	for(const Mortgage &mortgage : mortgages)
	{
		payments += mortgage.PrecisePayment();
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



// Get the total amount owed for a specific type of mortgage, or all
// mortgages if a blank string is provided.
int64_t Account::TotalDebt(const string &type) const
{
	int64_t total = 0;
	for(const Mortgage &mortgage : mortgages)
		if(type.empty() || mortgage.Type() == type)
			total += mortgage.Principal();

	return total;
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
