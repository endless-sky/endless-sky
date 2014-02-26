/* Account.h
Michael Zahniser, 20 Jan 2014

Class representing all your assets and liabilities and tracking their change
over time.
*/

#ifndef ACCOUNT_H_INCLUDED
#define ACCOUNT_H_INCLUDED

#include "DataFile.h"
#include "Mortgage.h"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

class Ship;



class Account {
public:
	// Default constructor.
	Account();
	
	// Load or save account data.
	void Load(const DataFile::Node &node);
	void Save(std::ostream &out) const;
	
	// Get or change the player's credits.
	int Credits() const;
	void AddCredits(int value);
	void PayExtra(int mortgage, int amount);
	
	// Step forward one day, and return a string summarizing payments made.
	std::string Step();
	
	// Liabilities:
	const std::vector<Mortgage> &Mortgages() const;
	void AddMortgage(int principal);
	void AddFine(int amount);
	int Prequalify() const;
	int Salaries() const;
	// Assets:
	int NetWorth() const;
	const std::vector<int> History() const;
	int YearlyRevenue() const;
	
	// Find out the player's credit rating.
	int CreditScore() const;
	
	
private:
	int credits;
	int salariesOwed;
	
	std::vector<Mortgage> mortgages;
	
	std::vector<int> history;
	int creditScore;
};



#endif
