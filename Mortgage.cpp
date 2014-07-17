/* Mortgage.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Mortgage.h"

#include "DataNode.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Find out how much you can afford to borrow with the given annual revenue
// and the given credit score (which should be between 200 and 800).
int Mortgage::Maximum(int annualRevenue, int creditScore, int term)
{
	double interest = (500 - creditScore / 2) * .00001;
	double power = pow(1. + interest, term);
	double multiplier = interest * term * power / (power - 1.);
	return static_cast<int>(max(0., annualRevenue / multiplier));
}



// Create a new mortgage of the given amount.
Mortgage::Mortgage(int principal, int creditScore, int term)
	: type(creditScore ? "Mortgage" : "Fine"),
	principal(principal),
	interest((500 - creditScore / 2) * .00001),
	interestString("0." + to_string(500 - creditScore / 2) + "%"),
	term(term)
{
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
		if(child.Token(0) == "principal" && child.Size() >= 2)
			principal = child.Value(1);
		else if(child.Token(0) == "interest" && child.Size() >= 2)
		{
			interest = child.Value(1);
			int f = 100000. * interest;
			interestString = "0." + to_string(f) + "%";
		}
		else if(child.Token(0) == "term" && child.Size() >= 2)
			term = child.Value(1);
	}
}



void Mortgage::Save(ostream &out) const
{
	out << "\tmortgage \"" << type << "\"\n";
	out << "\t\tprincipal " << principal << "\n";
	out << "\t\tinterest " << interest << "\n";
	out << "\t\tterm " << term << "\n";
}



// Make a mortgage payment. The return value is the amount paid.
int Mortgage::MakePayment()
{
	int payment = Payment();
	MissPayment();
	principal -= payment;
	--term;
	
	return payment;
}



void Mortgage::MissPayment()
{
	principal += static_cast<int>(principal * interest + .5);
}



// Pay down additional principal. Unlike a "real" mortgage, this reduces
// the minimum amount of your future payments, not the term of the mortgage.
// This returns the actual amount paid, which may be less if the total
// principal remaining is less than the given amount.
int Mortgage::PayExtra(int amount)
{
	amount = min(principal, amount);
	principal -= amount;
	return amount;
}



// The type is "Mortgage" if this is a mortgage you applied for from a bank,
// and "Fine" if this is a fine imposed on you for illegal activities.
const string &Mortgage::Type() const
{
	return type;
}



// Get the remaining mortgage principal.
int Mortgage::Principal() const
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
int Mortgage::Payment() const
{
	double power = pow(1. + interest, term);
	return round(principal * interest * power / (power - 1.));
}
