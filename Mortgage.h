/* Mortgage.h
Michael Zahniser, 12 Jan 2014

Class representing a mortgage (or a fine that can be paid in installments).
*/

#ifndef MORTGAGE_H_INCLUDED
#define MORTGAGE_H_INCLUDED

#include <string>



class Mortgage {
public:
	// Find out how much you can afford to borrow with the given annual revenue
	// and the given credit score (which should be between 200 and 800).
	static int Maximum(int annualRevenue, int creditScore, int term = 365);
	
	
public:
	// Create a new mortgage of the given amount. If this is a fine, set the
	// credit score to zero for a higher interest rate.
	Mortgage(int principal, int creditScore, int term = 365);
	
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
