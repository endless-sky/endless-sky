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

#include <cstdint>
#include <string>
#include <vector>

class DataNode;
class DataWriter;



// Class representing all your assets and liabilities and tracking their change
// over time.
class Account {
public:
	// Load or save account data.
	void Load(const DataNode &node, bool clearFirst);
	void Save(DataWriter &out) const;
	
	// Get or change the player's credits.
	int64_t Credits() const;
	void AddCredits(int64_t value);
	void PayExtra(int mortgage, int64_t amount);
	
	// Step forward one day, and return a string summarizing payments made.
	std::string Step(int64_t assets, int64_t salaries, int64_t maintenance);
	
	// Overdue crew salaries:
	int64_t SalariesOwed() const;
	void PaySalaries(int64_t amount);
	// Overdue maintenance costs:
	int64_t MaintenanceDue() const;
	void PayMaintenance(int64_t amount);
	
	// Liabilities:
	const std::vector<Mortgage> &Mortgages() const;
	void AddMortgage(int64_t principal);
	void AddFine(int64_t amount);
	int64_t Prequalify() const;
	// Assets:
	int64_t NetWorth() const;
	
	// Find out the player's credit rating.
	int CreditScore() const;
	// Get the total amount owed for "Mortgage", "Fine", or both.
	int64_t TotalDebt(const std::string &type = "") const;
	
	
private:
	int64_t YearlyRevenue() const;
	
	
private:
	int64_t credits = 0;
	// If back salaries and maintenance cannot be paid, they pile up rather
	// than being ignored.
	int64_t salariesOwed = 0;
	int64_t maintenanceDue = 0;
	// Your credit score determines the interest rate on your mortgages.
	int creditScore = 400;
	
	std::vector<Mortgage> mortgages;
	
	// History of the player's net worth. This is used to calculate your average
	// daily income, which is used to calculate how big a mortgage you can afford.
	std::vector<int64_t> history;
};



#endif
