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



void Account::Load(const DataNode &node, bool clearFirst)
{
	if(clearFirst)
	{
		credits = 0;
		overdueCrewSalaries = 0;
		overdueMaintenance = 0;
		creditScore = 400;
		mortgages.clear();
		history.clear();
	}

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "credits" && child.Size() >= 2)
			credits = child.Value(1);
		else if(child.Token(0) == "salaries income")
			for(const DataNode &grand : child)
			{
				if(grand.Size() < 2)
					grand.PrintTrace("Skipping incomplete salary income:");
				else
					salariesIncome[grand.Token(0)] = grand.Value(1);
			}
		else if(child.Token(0) == "salaries" && child.Size() >= 2)
			overdueCrewSalaries = child.Value(1);
		else if(child.Token(0) == "maintenance" && child.Size() >= 2)
			overdueMaintenance = child.Value(1);
		else if(child.Token(0) == "score" && child.Size() >= 2)
			creditScore = child.Value(1);
		else if(child.Token(0) == "mortgage")
			mortgages.emplace_back(child);
		else if(child.Token(0) == "history")
			for(const DataNode &grand : child)
				history.push_back(grand.Value(0));
		else
			child.PrintTrace("Skipping unrecognized account item:");
	}
}



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
		if(overdueCrewSalaries)
			out.Write("salaries", overdueCrewSalaries);
		if(overdueMaintenance)
			out.Write("maintenance", overdueMaintenance);
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



int64_t Account::Credits() const
{
	return credits;
}



void Account::SetCredits(int64_t value)
{
	credits = value;
}



void Account::AddCredits(int64_t value)
{
	credits += value;
}



int Account::CreditScore() const
{
	return creditScore;
}



const vector<int64_t> &Account::History() const
{
	return history;
}



int64_t Account::NetWorth() const
{
	return history.empty() ? 0 : history.back();
}



void Account::AddHistory(int64_t amount)
{
	history.push_back(amount);
	if(history.size() > HISTORY)
		history.erase(history.begin());
}



const vector<Mortgage> &Account::Mortgages() const
{
	return mortgages;
}



void Account::AddMortgage(int64_t principal)
{
	mortgages.emplace_back(principal, creditScore);
	credits += principal;
}



// Add a mortgage with an interest rate of zero and a short term.
void Account::AddFine(int64_t amount)
{
	mortgages.emplace_back(amount, 0, 60);
}



void Account::PayExtra(int mortgage, int64_t amount)
{
	if(static_cast<unsigned>(mortgage) >= mortgages.size()
		|| amount > credits
		|| amount > mortgages[mortgage].Principal())
	{
		return;
	}

	mortgages[mortgage].PayExtra(amount);
	credits -= amount;

	// If this payment was for the entire remaining amount in the mortgage,
	// remove it from the list.
	if(!mortgages[mortgage].Principal())
		mortgages.erase(mortgages.begin() + mortgage);
}



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



// Sum the principals of all mortgages of a given type and return the result
int64_t Account::TotalDebt(const string &type) const
{
	int64_t total = 0;
	for(const Mortgage &mortgage : mortgages)
		if(type.empty() || mortgage.Type() == type)
			total += mortgage.Principal();

	return total;
}



int64_t Account::OverdueCrewSalaries() const
{
	return overdueCrewSalaries;
}



void Account::SetOverdueCrewSalaries(int64_t value)
{
	overdueCrewSalaries = value;
}



// If the amount in overdue crew salaries is less then the amount passed,
// pay the overdue crew salaries. If either of those is more than the total
// number of credits in the players account, pay of as much as the player has.
void Account::PayOverdueCrewSalaries(int64_t amount)
{
	amount = min(min(amount, overdueCrewSalaries), credits);
	credits -= amount;
	overdueCrewSalaries -= amount;
}



int64_t Account::OverdueMaintenance() const
{
	return overdueMaintenance;
}



void Account::SetOverdueMaintenance(int64_t value)
{
	overdueMaintenance = value;
}



// If the amount in overdue maintenance is less then the amount passed,
// pay the overdue maintenance. If either of those is more than the total
// number of credits in the players account, pay of as much as the player has.
void Account::PayOverdueMaintenance(int64_t amount)
{
	amount = min(min(amount, overdueMaintenance), credits);
	credits -= amount;
	overdueMaintenance -= amount;
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
		[](int64_t value, const std::map<string, int64_t>::value_type &salary)
		{
			return value + salary.second;
		}
	);
}



void Account::SetSalariesIncome(const string &name, int64_t amount)
{
	if(amount == 0)
		salariesIncome.erase(name);
	else
		salariesIncome[name] = amount;
}



string Account::Step(int64_t assets, int64_t salaries, int64_t maintenance)
{
	ostringstream out;

	// Step 1: Pay the bills
	std::vector<Receipt> receipts = PayBills(salaries, maintenance);
	out << GenerateMissedPaymentLogs(&receipts);

	// If any mortgage has been fully paid off, remove it from the list.
	UpdateMortgages();

	AddHistory(CalculateNetWorth(assets));
	UpdateCreditScore(&receipts);

	// If you didn't make any payments, no need to continue further.
	if(!AnyPaymentsMade(&receipts))
		return out.str();

	out << GeneratePaymentLogs(&receipts);
	return out.str();
}



std::vector<Receipt> Account::PayBills(int64_t salaries, int64_t maintenance)
{
	std::vector<Receipt> receipts;

	// Crew salaries take highest priority.
	receipts.push_back(PayCrewSalaries(salaries));

	// Maintenance costs are dealt with after crew salaries given that they act similarly.
	receipts.push_back(PayMaintenance(maintenance));

	// Unlike salaries, each mortgage payment must either be made in its entirety,
	// or skipped completely (accruing interest and reducing your credit score).
	receipts.push_back(PayMortgages());
	receipts.push_back(PayFines());

	return receipts;
}



Receipt Account::PayCrewSalaries(int64_t salaries)
{
	Receipt receipt;

	overdueCrewSalaries += salaries;
	int64_t salariesPaid = overdueCrewSalaries;
	if(overdueCrewSalaries)
	{
		if(overdueCrewSalaries > credits)
		{
			// If you can't pay the full salary amount, still pay some of it and
			// remember how much back wages you owe to your crew.
			salariesPaid = max<int64_t>(credits, 0);
			overdueCrewSalaries -= salariesPaid;
			credits -= salariesPaid;
			receipt.paidInFull = false;
		}
		else
		{
			credits -= overdueCrewSalaries;
			overdueCrewSalaries = 0;
		}
	}

	receipt.creditsPaid = salariesPaid;
	return receipt;
}



Receipt Account::PayMaintenance(int64_t maintenance)
{
	Receipt receipt;

	overdueMaintenance += maintenance;
	int64_t maintenancePaid = overdueMaintenance;
	if(overdueMaintenance)
	{
		if(overdueMaintenance > credits)
		{
			// Like with crew salaries, maintenance costs can be paid in part with
			// the unpaid costs being paid later.
			maintenancePaid = max<int64_t>(credits, 0);
			overdueMaintenance -= maintenancePaid;
			credits -= maintenancePaid;
			receipt.paidInFull = false;
		}
		else
		{
			credits -= overdueMaintenance;
			overdueMaintenance = 0;
		}
	}

	receipt.creditsPaid = maintenancePaid;
	return receipt;
}



Receipt Account::PayMortgages()
{
	// This function needs to preserve:
	//    1. The number of credits paid towards ALL mortgages
	//    2. Whether ANY mortgage was not paid in full
	Receipt receipt;
	if(mortgages.empty()) return receipt;

	for(Mortgage &mortgage : mortgages)
	{
		if(!(mortgage.Type() == "Mortgage")) continue;
		int64_t payment = mortgage.Payment();
		if(payment > credits)
		{
			mortgage.MissPayment();
			receipt.paidInFull = false;
		}
		else
		{
			payment = mortgage.MakePayment();
			credits -= payment;
			receipt.creditsPaid += payment;
		}
	}

	return receipt;
}



Receipt Account::PayFines()
{
	// This function needs to preserve:
	//    1. The number of credits paid towards ALL fines
	//    2. Whether ANY fine was not paid in full
	Receipt receipt;
	if(mortgages.empty()) return receipt;

	for(Mortgage &mortgage : mortgages)
	{
		if(!(mortgage.Type() == "Fine")) continue;
		int64_t payment = mortgage.Payment();
		if(payment > credits)
		{
			mortgage.MissPayment();
			receipt.paidInFull = false;
		}
		else
		{
			payment = mortgage.MakePayment();
			credits -= payment;
			receipt.creditsPaid += payment;
		}
	}

	return receipt;
}



const string Account::GenerateMissedPaymentLogs(std::vector<Receipt> *receipts) const
{
	ostringstream log;

	if(!receipts->at(0).paidInFull)
		log << "You could not pay all your crew salaries. ";

	if(!receipts->at(1).paidInFull)
		log << "You could not pay all your maintenance costs. ";

	if(Mortgages().size() > 0)
	{
		if(!receipts->at(2).paidInFull || !receipts->at(3).paidInFull)
			log << "You missed a mortgage payment. ";
	}

	return log.str();
}



void Account::UpdateMortgages()
{
	for(auto it = mortgages.begin(); it != mortgages.end(); )
	{
		if(!it->Principal())
			it = mortgages.erase(it);
		else
			++it;
	}
}



int64_t Account::CalculateNetWorth(int64_t assets) const
{
	return assets - TotalDebt() - OverdueCrewSalaries() - OverdueMaintenance();
}



void Account::UpdateCreditScore(std::vector<Receipt> *receipts)
{
	// If you failed to pay any debt, your credit score drops. Otherwise, even
	// if you have no debts, it increases. (Because, having no debts at all
	// makes you at least as credit-worthy as someone who pays debts on time.)
	bool missedPayment = false;
	for(Receipt receipt : *receipts)
	{
		if(!receipt.paidInFull)
		{
			missedPayment = true;
			break;
		}
	}
	creditScore = max(200, min(800, creditScore + (missedPayment ? -5 : 1)));
}



bool Account::AnyPaymentsMade(std::vector<Receipt> *receipts)
{
	for(Receipt receipt : *receipts)
	{
		if(receipt.creditsPaid > 0)
			return true;
	}
	return false;
}



string Account::GeneratePaymentLogs(std::vector<Receipt> *receipts)
{
	ostringstream log;
	log << "You paid ";
	map<string, int64_t> typesPaid = Account::GetTypesPaid(receipts);

	// If you made payments of three or more types, the punctuation needs to
	// include commas, so just handle that separately here.
	if(typesPaid.size() >= 3)
	{
		auto it = typesPaid.begin();
		for(unsigned int i = 0; i < typesPaid.size() - 1; ++i)
		{
			log << Format::CreditString(it->second) << " in " << it->first << ", ";
			++it;
		}
		log << "and " << Format::CreditString(it->second) << " in " << it->first + ".";
	}
	else
	{
		if(receipts->at(0).creditsPaid > 0)
			log << Format::CreditString(receipts->at(0).creditsPaid) << " in crew salaries"
				<< ((receipts->at(2).creditsPaid
					|| receipts->at(3).creditsPaid
					|| receipts->at(1).creditsPaid > 0) ? " and " : ".");
		if(receipts->at(1).creditsPaid > 0)
			log << Format::CreditString(receipts->at(1).creditsPaid) << " in maintenance"
				<< ((receipts->at(2).creditsPaid || receipts->at(3).creditsPaid) ? " and " : ".");
		if(receipts->at(2).creditsPaid)
			log << Format::CreditString(receipts->at(2).creditsPaid) << " in mortgages"
				<< (receipts->at(3).creditsPaid ? " and " : ".");
		if(receipts->at(3).creditsPaid)
			log << Format::CreditString(receipts->at(3).creditsPaid) << " in fines.";
	}

	return log.str();
}



map<string, int64_t> Account::GetTypesPaid(std::vector<Receipt> *receipts)
{
	map<string, int64_t> typesPaid;

	if(receipts->at(0).creditsPaid > 0)
		typesPaid["crew salaries"] = receipts->at(0).creditsPaid;
	if(receipts->at(1).creditsPaid > 0)
		typesPaid["maintenance"] = receipts->at(1).creditsPaid;
	if(receipts->at(2).creditsPaid > 0)
		typesPaid["mortgages"] = receipts->at(2).creditsPaid;
	if(receipts->at(3).creditsPaid > 0)
		typesPaid["fines"] = receipts->at(3).creditsPaid;

	return typesPaid;
}



int64_t Account::YearlyRevenue() const
{
	if(history.empty() || history.back() <= history.front())
		return 0;

	// Note that this intentionally under-estimates if the player has not yet
	// played for long enough to accumulate a full income history.
	return ((history.back() - history.front()) * 365) / HISTORY;
}
