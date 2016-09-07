/* Date.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Date.h"

using namespace std;

namespace {
	// Figure out the day of the week of the given date.
	const string &Weekday(int day, int month, int year)
	{
		// Zeller's congruence.
		if(month < 3)
		{
			--year;
			month += 12;
		}
		day = (day + (13 * (month + 1)) / 5 + year + year / 4 + 6 * (year / 100) + year / 400) % 7; 

		static const string DAY[] = {"Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"};
		return DAY[day];
	}
}



// Since converting a date to a string is the most common operation, store the
// date in a way that allows easy extraction of the day, month, and year. Allow
// 5 bits for the day and 4 for the month. This also allows easy comparison.
Date::Date(int day, int month, int year)
	: date(day + (month << 5) + (year << 9))
{
}



// Convert a date to a string.
const string &Date::ToString() const
{
	// Because this is a somewhat "costly" operation, cache the result. The
	// cached value is discarded if the date is changed.
	if(date && str.empty())
	{
		int day = Day();
		int month = Month();
		int year = Year();
	
		str = Weekday(day, month, year);
		str.append(", ");
		str.append(to_string(day));
		str.append(" ");
		static const string MONTH[] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		str.append(MONTH[month - 1]);
		str.append(" ");
		str.append(to_string(year));
	}
	
	return str;
}



// Convert a date to the format in which it would be stated in conversation.
string Date::LongString() const
{
	if(!date)
		return string();
	
	int day = Day();
	string result = "the " + to_string(day);
	// All numbers in the teens add in "th", as do any numbers ending in 0 or in
	// 4 through 9. Special endings are used for "1st", "2nd", and "3rd."
	if(day / 10 == 1 || day % 10 == 0 || day % 10 > 3)
		result += "th";
	else if(day % 10 == 1)
		result += "st";
	else if(day % 10 == 2)
		result += "nd";
	else
		result += "rd";
	
	// Write out the month name instead of abbreviating it.
	result += " of ";
	static const string MONTH[] = {
		"January",
		"February",
		"March",
		"April",
		"May",
		"June",
		"July",
		"August",
		"September",
		"October",
		"November",
		"December"
	};
	result += MONTH[Month() - 1];
	
	return result;
}



// Check if this date has been initialized.
Date::operator bool() const
{
	return !!*this;
}



// Check if this date has not been initialized.
bool Date::operator!() const
{
	return !date;
}



// Increment this date.
void Date::operator++()
{
	*this = Date(*this + 1);
}



// Increment this date.
void Date::operator++(int)
{
	++*this;
}



// Add the given number of days to this date.
Date Date::operator+(int days) const
{
	// If this date is not initialized, adding to it does nothing.
	if(!date || !days)
		return *this;
	
	int day = Day();
	int month = Month();
	int year = Year();
	
	day += days;
	int leap = !(year % 4) - !(year % 100) + !(year % 400);
	int MDAYS[] = {31, 28 + leap, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	while(day > MDAYS[month - 1])
	{
		// Moving forward in time:
		day -= MDAYS[month - 1];
		++month;
		if(month == 13)
		{
			month = 1;
			++year;
			// If we cycle to a new year, recalculate the days in February.
			MDAYS[1] = 28 + !(year % 4) - !(year % 100) + !(year % 400);
		}
	}
	while(day < 1)
	{
		// Moving backward in time:
		--month;
		if(month == 0)
		{
			month = 12;
			--year;
			// If we cycle to a new year, recalculate the days in February.
			MDAYS[1] = 28 + !(year % 4) - !(year % 100) + !(year % 400);
		}
		day += MDAYS[month - 1];
	}
	return Date(day, month, year);
}



// Get the number of days between the two given dates.
int Date::operator-(const Date &other) const
{
	return DaysSinceEpoch() - other.DaysSinceEpoch();
}



// Date comparison.
bool Date::operator<(const Date &other) const
{
	return date < other.date;
}



bool Date::operator<=(const Date &other) const
{
	return date <= other.date;
}



bool Date::operator>(const Date &other) const
{
	return date > other.date;
}



bool Date::operator>=(const Date &other) const
{
	return date >= other.date;
}



bool Date::operator==(const Date &other) const
{
	return date == other.date;
}



bool Date::operator!=(const Date &other) const
{
	return date != other.date;
}



// Get the number of days that have elapsed since the "epoch". This is used only
// for finding the number of days in between two dates.
int Date::DaysSinceEpoch() const
{
	if(date && !daysSinceEpoch)
	{
		daysSinceEpoch = Day();
		int month = Month();
		int year = Year();
		
		// Months contain a variable number of days.
		int MDAYS[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
		daysSinceEpoch += MDAYS[month - 1];
		// Add in a leap day if this is a leap year and it is after February.
		if(month > 2 && !(year % 4) && ((year % 100) || !(year % 400)))
			++daysSinceEpoch;
		
		// Simplify the calculations by starting from year 1, so that leap years
		// occur at the very end of the cycle.
		--year;
		
		// Every four centuries is 365.2425*400 = 146097 days.
		daysSinceEpoch += 146097 * (year / 400);
		year %= 400;
	
		// Every century since the last one divisible by 400 contains 36524 days.
		daysSinceEpoch += 36524 * (year / 100);
		year %= 100;
	
		// Every four years since the century contain 4 * 365 + 1 = 1461 days.
		daysSinceEpoch += 1461 * (year / 4);
		year %= 4;
	
		// Every year since the last leap year contains 365 days.
		daysSinceEpoch += 365 * year;
	}
	return daysSinceEpoch;
}



// Get the current day of the month.
int Date::Day() const
{
	return (date & 31);
}



// Get the current month (January = 1, rather than being zero-indexed).
int Date::Month() const
{
	return ((date >> 5) & 15);
}



// Get the current year.
int Date::Year() const
{
	return (date >> 9);
}
