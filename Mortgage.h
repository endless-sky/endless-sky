/* Mortgage.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef MORTGAGE_H_
#define MORTGAGE_H_

#include "DataFile.h"

#include <string>



// Class representing a mortgage (or a fine that can be paid in installments).
class Mortgage {
public:
	// Find out how much you can afford to borrow with the given annual revenue
	// and the given credit score (which should be between 200 and 800).
	static int Maximum(int annualRevenue, int creditScore, int term = 365);
	
	
public:
	// Create a new mortgage of the given amount. If this is a fine, set the
	// credit score to zero for a higher interest rate.
	Mortgage(int principal, int creditScore, int term = 365);
	
	// Load or save mortgage data.
	void Load(const DataFile::Node &node);
	void Save(std::ostream &out) const;
	
	// Make a mortgage payment. The return value is the amount paid.
	int MakePayment();
	void MissPayment();
	// Pay down additional principal. Unlike a "real" mortgage, this reduces
	// the minimum amount of your future payments, not the term of the mortgage.
	// This returns the actual amount paid, which may be less if the total
	// principal remaining is less than the given amount.
	int PayExtra(int amount);
	
	// The type is "Mortgage" if this is a mortgage you applied for from a bank,
	// and "Fine" if this is a fine imposed on you for illegal activities.
	const std::string &Type() const;
	// Get the remaining mortgage principal.
	int Principal() const;
	// Get the interest rate. It is formatted as a string, because all that the
	// program will ever do with this is display it.
	const std::string &Interest() const;
	// Get the remaining number of payments that must be made.
	int Term() const;
	// Check the amount of the next payment due (rounded to the nearest credit).
	int Payment() const;
	
	
private:
	// Note: once a mortgage is set up, only the principal and term will change.
	std::string type;
	int principal;
	double interest;
	std::string interestString;
	int term;
};



#endif
