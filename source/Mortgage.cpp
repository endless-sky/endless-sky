/* Mortgage.cpp
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

#include "Mortgage.h"

#include "DataNode.h"
#include "DataWriter.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	const int MORTGAGE_TERM = 365;
}



// Find out how much you can afford to borrow with the given annual revenue
// and the given credit score (which should be between 200 and 800).
int64_t Mortgage::Maximum(int64_t annualRevenue, int creditScore, double currentPayments)
{
	const double revenue = annualRevenue - MORTGAGE_TERM * currentPayments;
	if(revenue <= 0.)
		return 0;

	const double interest = (600 - creditScore / 2) * .00001;
	const double power = pow(1. + interest, MORTGAGE_TERM);
	const double multiplier = interest * MORTGAGE_TERM * power / (power - 1.);
	return static_cast<int64_t>(max(0., revenue / multiplier));
}



// Create a new mortgage of the given amount.
Mortgage::Mortgage(string type, int64_t principal, int creditScore, int term)
	: type(std::move(type)),
	principal(principal),
	interest((600 - creditScore / 2) * .00001),
	interestString("0." + to_string(600 - creditScore / 2) + "%"),
	term(term)
{
}



// Create a mortgage with a specific interest rate instead of using the player's
// credit score. Due to how the class is set up, the interest rate must currently
// be within the range [0, 1).
Mortgage::Mortgage(string type, int64_t principal, double interest, int term)
	: type(std::move(type)),
	principal(principal),
	interest(interest * .01),
	interestString("0." + to_string(static_cast<int>(1000. * interest)) + "%"),
	term(term)
{
}



// Construct and Load() at the same time.
Mortgage::Mortgage(const DataNode &node)
{
	Load(node);
}



// Load or save mortgage data.
void Mortgage::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		type = node.Token(1);
	else
		type = "Mortgage";

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "principal" && hasValue)
			principal = child.Value(1);
		else if(key == "interest" && hasValue)
		{
			interest = child.Value(1);
			int f = 100000. * interest;
			interestString = "0." + to_string(f) + "%";
		}
		else if(key == "term" && hasValue)
			term = max(1., child.Value(1));
	}
}



void Mortgage::Save(DataWriter &out) const
{
	out.Write("mortgage", type);
	out.BeginChild();
	{
		out.Write("principal", principal);
		out.Write("interest", interest);
		out.Write("term", term);
	}
	out.EndChild();
}



// Make a mortgage payment. The return value is the amount paid.
int64_t Mortgage::MakePayment()
{
	int64_t payment = Payment();
	MissPayment();
	principal -= payment;
	--term;

	return payment;
}



void Mortgage::MissPayment()
{
	principal += lround(principal * interest);
}



// Pay down additional principal. Unlike a "real" mortgage, this reduces
// the minimum amount of your future payments, not the term of the mortgage.
// This returns the actual amount paid, which may be less if the total
// principal remaining is less than the given amount.
int64_t Mortgage::PayExtra(int64_t amount)
{
	amount = min(principal, amount);
	principal -= amount;
	return amount;
}



// The type is "Mortgage" if this is a mortgage you applied for from a bank,
// "Fine" if this is a fine imposed on you for illegal activities, and
// "Debt" if this is debt given to you by a mission.
const string &Mortgage::Type() const
{
	return type;
}



// Get the remaining mortgage principal.
int64_t Mortgage::Principal() const
{
	return principal;
}



// Get the interest rate. It is formatted as a string, because all that the
// program will ever do with this is display it.
const string &Mortgage::Interest() const
{
	return interestString;
}



// Get the remaining number of payments that must be made.
int Mortgage::Term() const
{
	return term;
}



// Check the amount of the next payment due (rounded to the nearest credit).
int64_t Mortgage::Payment() const
{
	if(!term)
		return principal;
	if(!interest)
		return lround(static_cast<double>(principal) / term);

	// Always require every payment to be at least 1 credit.
	double power = pow(1. + interest, term);
	return max<int64_t>(1, lround(principal * interest * power / (power - 1.)));
}



// Check the amount of the next payment due.
double Mortgage::PrecisePayment() const
{
	if(!term)
		return principal;
	if(!interest)
		return static_cast<double>(principal) / term;

	const double power = pow(1. + interest, term);
	return principal * interest * power / (power - 1.);
}
