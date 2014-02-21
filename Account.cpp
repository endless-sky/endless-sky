/* Account.cpp
Michael Zahniser, 20 Jan 2014

Function definitions for the Account class.
*/

#include "Account.h"

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
string Account::Step()
{
	ostringstream out;
	
	salariesOwed += Salaries();
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
			credits -= salariesOwed;
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
	}
	for(auto it = mortgages.begin(); it != mortgages.end(); )
	{
		if(!it->Principal())
			it = mortgages.erase(it);
		else
			++it;
	}
	
	// TODO: calculate net worth of all ships.
	int assets = 0;
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
				"credits in crew salaries and " : "credits in crew salaries.");
		if(mortgagesPaid)
			out << mortgagesPaid << (salariesPaid ? " " : " credits ")
				<< (finesPaid ? "in mortgage payments and " : "in mortgage payments.");
		if(finesPaid)
			out << finesPaid << ((salariesPaid || mortgagesPaid) ?
				" in fines." : " credits in fines.");
	}
	return out.str();
}



// Give the Account a reference to the list of ships owned by the player, so
// it can calculate crew salaries and net worth.
void Account::AddAsset(shared_ptr<const Ship *> ship)
{
	ships.push_back(ship);
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



int Account::Salaries() const
{
	// One crew member is the player themself.
	int crew = -1;
	for(auto ptr : ships)
		if(ptr && *ptr)
			crew += (**ptr).Crew();
	return (crew > 0) ? 100 * crew : 0;
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
