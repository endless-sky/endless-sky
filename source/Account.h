/* Account.h
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

#pragma once

#include "Mortgage.h"

#include <cstdint>
#include <map>
#include <optional>
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

	// Structural income.
	const std::map<std::string, int64_t> &SalariesIncome() const;
	int64_t SalariesIncomeTotal() const;
	void SetSalaryIncome(const std::string &name, int64_t amount);

	// Overdue crew salaries:
	int64_t CrewSalariesOwed() const;
	void PaySalaries(int64_t amount);
	// Overdue maintenance costs:
	int64_t MaintenanceDue() const;
	void PayMaintenance(int64_t amount);

	// Liabilities:
	const std::vector<Mortgage> &Mortgages() const;
	void AddMortgage(int64_t principal);
	void AddFine(int64_t amount);
	void AddDebt(int64_t amount, std::optional<double> interest, int term);
	int64_t Prequalify() const;
	// Assets:
	int64_t NetWorth() const;

	// Find out the player's credit rating.
	int CreditScore() const;
	// Get the total amount owed for a specific type of mortgage, or all
	// mortgages if a blank string is provided.
	int64_t TotalDebt(const std::string &type = "") const;


private:
	int64_t YearlyRevenue() const;


private:
	int64_t credits = 0;
	// Regular income from salaries paid to the player.
	std::map<std::string, int64_t> salariesIncome;
	// If back salaries and maintenance cannot be paid, they pile up rather
	// than being ignored.
	int64_t crewSalariesOwed = 0;
	int64_t maintenanceDue = 0;
	// Your credit score determines the interest rate on your mortgages.
	int creditScore = 400;

	std::vector<Mortgage> mortgages;

	// History of the player's net worth. This is used to calculate your average
	// daily income, which is used to calculate how big a mortgage you can afford.
	std::vector<int64_t> history;
};
