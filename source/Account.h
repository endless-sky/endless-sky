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
#include <tuple>
#include <vector>

class DataNode;
class DataWriter;

// A utility structure to return multiple values needed during processing
// inside the Step function and its subprocesses.
struct Receipt {
	bool paidInFull = true;
	int64_t creditsPaid = 0;
};


// Class representing all your assets and liabilities and tracking their change
// over time.
class Account {
public:
	// Load account information from a data file (saved game or starting conditions).
	void Load(const DataNode &node, bool clearFirst);

	// Write account information to a saved game file.
	void Save(DataWriter &out) const;

	// Get how many credits the player currently has in the bank.
	int64_t Credits() const;

	// Set the number of credits in the players account.
	void SetCredits(int64_t value);

	// Give the player credits (or pass negative number to subtract). If subtracting,
	// the calling function needs to check that this will not result in negative credits.
	void AddCredits(int64_t value);

	// Get the player's credit rating.
	int CreditScore() const;

	// Access the history of the player's net worth.
	const std::vector<int64_t> &History() const;

	// Get the player's total net worth (counting all ships and all debts).
	int64_t NetWorth() const;

	// Append an amount of credits to a player's history of net worth.
	// If the length exceeds the allowed length, the earliest entry will
	// be deleted.
	void AddHistory(int64_t amount);

	// Access the list of mortgages.
	const std::vector<Mortgage> &Mortgages() const;

	// Add a new mortgage for the given amount, with an interest rate determined by your credit score.
	void AddMortgage(int64_t principal);

	// Add a "fine" as a mortgage.
	void AddFine(int64_t amount);

	// Add a "debt" as a mortgage.
	void AddDebt(int64_t amount, std::optional<double> interest, int term);

	// Pay down extra principal on a mortgage.
	void PayExtra(int mortgage, int64_t amount);

	// Check how big a mortgage the player can afford to pay at their current income.
	int64_t Prequalify() const;

	// Get the total amount owed for a specific type of mortgage, or all
	// mortgages if a blank string is provided.
	int64_t TotalDebt(const std::string &type = "") const;

	// Access the overdue crew salaries.
	int64_t OverdueCrewSalaries() const;

	// Set the number of credits of unpaid salaries the player owes.
	void SetOverdueCrewSalaries(int64_t value);

	// Pay off the current overdue crew salaries by a given amount.
	void PayOverdueCrewSalaries(int64_t amount);

	// Access the overdue maintenance costs.
	int64_t OverdueMaintenance() const;

	// Set the number of credits of unpaid maintenance the player owes.
	void SetOverdueMaintenance(int64_t value);

	// Pay off the current overdue maintenance by a given amount.
	void PayOverdueMaintenance(int64_t amount);

	// Access the map of player salaries.
	const std::map<std::string, int64_t> &SalariesIncome() const;

	// Return the sum of all player salaries.
	int64_t SalariesIncomeTotal() const;

	// Set the number of credits in a specified salary. This will add the
	// salaries if it does not already exist, or modify and existing salary.
	// If the amount passed in is 0, then the salary will be removed.
	void SetSalariesIncome(const std::string &name, int64_t amount);

	// Step forward one day, and return a string summarizing payments made.
	// NOTE: This function may modify account data during operation.
	std::string Step(int64_t assets, int64_t salaries, int64_t maintenance);


private:
	// Attempt to pay crew salaries, returning the results.
	// NOTE: This function may modify the credits and overdueCrewSalaries
	// variables during execution.
	Receipt PayCrewSalaries(int64_t salaries);

	// Attempt to pay mainenance costs, returning the results.
	// NOTE: This function may modify the credits and overdueMaintenance
	// variables during execution.
	Receipt PayMaintenance(int64_t maintenance);

	// Attempt to mortgages, returning the results.
	// NOTE: This function may modify the credits variable during execution.
	Receipt PayMortgages();

	// Attempt to pay fines, returning the results.
	// NOTE: This function may modify the credits variable during execution.
	Receipt PayFines();

	// Attempt to pay debts, returning the results.
	// NOTE: This function may modify the credits variable during execution.
	Receipt PayDebts();

	// Generate a log string from the given receipts.
	const std::string GenerateMissedPaymentLogs(std::vector<Receipt> *receipts) const;

	// Update the lists of mortgages the player holds.
	// This will remove any mortgages with a principal of 0.
	void UpdateMortgages();

	// Calculate the player's net worth based on the given assets.
	int64_t CalculateNetWorth(int64_t assets) const;

	// Update the players credit score based on the given receipts.
	// The credit score will always be set to a number between 200
	// and 800, inclusive.
	void UpdateCreditScore(std::vector<Receipt> *receipts);

	// Check the receipts to see if any contained a non-zero payment.
	static bool AnyPaymentsMade(std::vector<Receipt> *receipts);

	// Generate a log string from the given receipts.
	static std::string GeneratePaymentLogs(std::vector<Receipt> *receipts);

	// Check the receipts to see what types fo payments were made.
	// If none were made, the map returned will be empty.
	static std::map<std::string, int64_t> GetTypesPaid(std::vector<Receipt> *receipts);

	// Extrapolate from the player's current net worth history to determine how much
	// their net worth is expected to change over the course of the next year.
	int64_t YearlyRevenue() const;


private:
	// The liquid cash a player has in their account.
	int64_t credits = 0;

	// Your credit score determines the interest rate on your mortgages.
	int creditScore = 400;

	// History of the player's net worth. This is used to calculate your average
	// daily income, which is used to calculate how big a mortgage you can afford.
	std::vector<int64_t> history;

	// A list containing mortgages taken out and fines assigned to the player.
	std::vector<Mortgage> mortgages;

	// If back salaries cannot be paid, it piles up rather than being ignored.
	int64_t overdueCrewSalaries = 0;

	// If back maintenance cannot be paid, it piles up rather than being ignored.
	int64_t overdueMaintenance = 0;

	// Regular income from salaries paid to the player.
	std::map<std::string, int64_t> salariesIncome;
};
