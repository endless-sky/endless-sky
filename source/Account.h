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

#ifndef ACCOUNT_H_
#define ACCOUNT_H_

#include "Mortgage.h"

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

class DataNode;
class DataWriter;

struct Receipt {
	bool paidInFull = true;
	int64_t creditsPaid = 0;
};


// Class representing all your assets and liabilities and tracking their change
// over time.
class Account {
public:
	// Load or save account data.
	void Load(const DataNode &node, bool clearFirst);
	void Save(DataWriter &out) const;

	// Functions operating on the player's credits.
	int64_t Credits() const;
	void SetCredits(int64_t value);
	void AddCredits(int64_t value);
	void SubtractCredits(int64_t value);
	void PayExtra(int mortgage, int64_t amount);

	// Functions operating on the player's credit score.
	// Getters, Setters, Modifiers
	// Find out the player's credit rating.
	int CreditScore() const;
	void SetCreditScore(int64_t value);

	// Functions operating on the player's crew salaries.
	// Getters, Setters, Modifiers

	// Functions operating on the player's history.
	// Getters, Setters, Modifiers

	// Functions operating on the player's maintenance.
	// Getters, Setters, Modifiers

	// Functions operating on the player's mortgages and fines.
	// Getters, Setters, Modifiers

	// Functions operating on the player's salaries.
	// Getters, Setters, Modifiers

	// Step forward one day, and return a string summarizing payments made.
	std::string Step(int64_t assets, int64_t salaries, int64_t maintenance);
	// Utility functions for Step
	std::vector<Receipt> PayBills(int64_t salaries, int64_t maintenance);
	Receipt PayCrewSalaries(int64_t salaries);
	Receipt PayShipMaintenance(int64_t maintenance);
	Receipt PayMortgages();
	Receipt PayFines();
	const std::string GenerateMissedPaymentLogs(std::vector<Receipt> *receipts) const;
	void UpdateMortgages();
	void UpdateHistory(int64_t assets);
	int64_t CalculateNetWorth(int64_t assets) const;
	void UpdateCreditScore(std::vector<Receipt> *receipts);
	static bool AnyPaymentsMade(std::vector<Receipt> *receipts);
	static std::map<std::string, int64_t> GetTypesPaid(std::vector<Receipt> *receipts);
	static std::string GeneratePaymentLogs(std::vector<Receipt> *receipts);

	// Structural income.
	const std::map<std::string, int64_t> &SalariesIncome() const;
	int64_t SalariesIncomeTotal() const;
	void SetSalaryIncome(std::string name, int64_t amount);

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
	int64_t Prequalify() const;
	// Assets:
	int64_t NetWorth() const;

	// Get the total amount owed for "Mortgage", "Fine", or both.
	int64_t TotalDebt(const std::string &type = "") const;

	const std::vector<int64_t> &History() const;


private:
	int64_t YearlyRevenue() const;


private:
	int64_t credits = 0;

	// Your credit score determines the interest rate on your mortgages.
	int creditScore = 400;

	// If back salaries cannot be paid, it piles up rather than being ignored.
	int64_t crewSalariesOwed = 0;

	// History of the player's net worth. This is used to calculate your average
	// daily income, which is used to calculate how big a mortgage you can afford.
	std::vector<int64_t> history;

	// If back maintenance cannot be paid, it piles up rather than being ignored.
	int64_t maintenanceDue = 0;

	// A list containing mortgages taken out and fines assigned to the player
	std::vector<Mortgage> mortgages;

	// Regular income from salaries paid to the player.
	std::map<std::string, int64_t> salariesIncome;
};



#endif
