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



const string &Date::ToString() const
{
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



string Date::LongString() const
{
	if(!date)
		return string();
	
	int day = Day();
	string result = "the " + to_string(day);
	if(day / 10 == 1 || day % 10 == 0 || day % 10 > 3)
		result += "th";
	else if(day % 10 == 1)
		result += "st";
	else if(day % 10 == 2)
		result += "nd";
	else
		result += "rd";
	
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



bool Date::operator!() const
{
	return !date;
}



void Date::operator++()
{
	*this = Date(*this + 1);
}



void Date::operator++(int)
{
	++*this;
}



Date Date::operator+(int days) const
{
	if(!date)
		return *this;
	
	int day = Day();
	int month = Month();
	int year = Year();
	
	day += days;
	int leap = !(year % 4) - !(year % 100) + !(year % 400);
	int MDAYS[] = {31, 28 + leap, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	while(day > MDAYS[month - 1])
	{
		day -= MDAYS[month - 1];
		++month;
		if(month == 13)
		{
			month = 1;
			++year;
			MDAYS[1] = 28 + !(year % 4) - !(year % 100) + !(year % 400);
		}
	}
	return Date(day, month, year);
}



int Date::operator-(const Date &other) const
{
	return DaysSinceEpoch() - other.DaysSinceEpoch();
}



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



// Get the number of days that have elapsed since the "epoch".
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



// Get the date as numbers.
int Date::Day() const
{
	return (date & 31);
}



int Date::Month() const
{
	return ((date >> 5) & 15);
}



int Date::Year() const
{
	return (date >> 9);
}
