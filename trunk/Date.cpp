/* Date.cpp
Michael Zahniser, 28 Oct 2013

Function definitions for the Date class.
*/

#include "Date.h"

#include <cstring>

using namespace std;

namespace {
	// Only allow Dates to step by whole-day increments.
	static const time_t SECONDS_PER_DAY = 60 * 60 * 24;
	static const double SECONDS_TO_DAYS = 1. / SECONDS_PER_DAY;
	
	// The string will never be longer than 29 characters, plus a '\0'.
	static const size_t MAX_SIZE = 32;
}



Date::Date(int day, int month, int year)
	: str(MAX_SIZE, '\0')
{
	tm t;
	memset(&t, 0, sizeof(t));
	t.tm_hour = 12;
	t.tm_mday = day;
	t.tm_mon = month - 1;
	t.tm_year = year - 1900;
	today = mktime(&t);
}



const string &Date::ToString() const
{
	if(str.size() == MAX_SIZE)
	{
		// Convert to a tm structure (day of the week, etc.).
		tm t;
		gmtime_r(&today, &t);
	
		size_t length = strftime(&str.front(), MAX_SIZE, "%a, %-d %b %Y", &t);
		str.resize(length);
	}
	
	return str;
}



void Date::operator++()
{
	today += SECONDS_PER_DAY;
	
	// Flag ToString() that the conversion must be done over.
	str.resize(MAX_SIZE);
}



void Date::operator++(int)
{
	++*this;
}



// Get the number of days that have elapsed since the "epoch".
double Date::DaysSinceEpoch() const
{
	return today * SECONDS_TO_DAYS;
}



// Get the date as numbers.
int Date::Day() const
{
	tm t;
	gmtime_r(&today, &t);
	
	return t.tm_mday;
}



int Date::Month() const
{
	tm t;
	gmtime_r(&today, &t);
	
	return t.tm_mon + 1;
}



int Date::Year() const
{
	tm t;
	gmtime_r(&today, &t);
	
	return t.tm_year + 1900;
}
