/* Date.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DATE_H_
#define DATE_H_

#include <string>



// Class representing a date (day, month, and year). Built-in date structures
// combined dates with time within the day, which can lead to accumulated errors
// when trying to increment dates in whole-day steps.
class Date {
public:
	Date() = default;
	Date(int day, int month, int year);
	
	// Get this date as a string, in the form "Day, DD Mon Year".
	const std::string &ToString() const;
	// Get a string in the form "the DDth of Month", suitable to include in
	// conversation text.
	std::string LongString() const;
	
	// Check if this date has been initialized.
	explicit operator bool() const;
	bool operator!() const;
	
	// Move the date forward one day.
	void operator++();
	void operator++(int);
	// Get the date this number of days in the future.
	Date operator+(int days) const;
	// Get the number of days between two dates.
	int operator-(const Date &other) const;
	// Comparison operators.
	bool operator<(const Date &other) const;
	bool operator<=(const Date &other) const;
	bool operator>(const Date &other) const;
	bool operator>=(const Date &other) const;
	bool operator==(const Date &other) const;
	bool operator!=(const Date &other) const;
	
	// Get the number of days that have elapsed since the "epoch".
	int DaysSinceEpoch() const;
	
	// Get the date as numbers.
	int Day() const;
	int Month() const;
	int Year() const;
	
	
private:
	// The date is compressed into a single integer value to make it easy to
	// compare dates.
	int date = 0;
	// Cached values of complex operations.
	mutable int daysSinceEpoch = 0;
	mutable std::string str;
};



#endif
