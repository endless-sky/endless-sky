/* Account.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ACCOUNT_H_
#define ACCOUNT_H_

#include "Mortgage.h"

#include <memory>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Ship;



// Class representing all your assets and liabilities and tracking their change
// over time.
class Account {
public:
	// Default constructor.
	Account();
	
	// Load or save account data.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;
	
	// Get or change the player's credits.
	int Credits() const;
	void AddCredits(int value);
	void PayExtra(int mortgage, int amount);
	
	// Step forward one day, and return a string summarizing payments made.
	std::string Step(int assets, int salaries);
	
	// Liabilities:
	const std::vector<Mortgage> &Mortgages() const;
	void AddMortgage(int principal);
	void AddFine(int amount);
	int Prequalify() const;
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
