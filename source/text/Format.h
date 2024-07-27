/* Format.h
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ES_TEXT_FORMAT_H_
#define ES_TEXT_FORMAT_H_

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>



// Collection of functions for formatting strings for display.
class Format {
public:
	// Function to retrieve a condition's value. Receives a string that contains
	// the condition name, and the start & size of the substring with the condition
	// name.
	using ConditionGetter = std::function<int64_t(const std::string &source, size_t start, size_t size)>;


public:
	// Convert the given number into abbreviated format with a suffix like
	// "M" for million, "B" for billion, or "T" for trillion. Any number
	// above 1 quadrillion is instead shown in scientific notation.
	static std::string Credits(int64_t value);
	// Convert the given number into abbreviated format as described in Format::Credits,
	// then attach the ' credit' or ' credits' suffix to it.
	static std::string CreditString(int64_t value);
	// Writes the given number into a string,
	// then attach the ' ton' or ' tons' suffix to it.
	static std::string MassString(double amount);
	// Creates a string similar to '<amount> tons of <cargo>'.
	static std::string CargoString(double amount, const std::string &cargo);
	// Convert a time in seconds to years/days/hours/minutes/seconds
	static std::string PlayTime(double timeVal);
	// Convert the given number to a string, with at most one decimal place.
	// This is primarily for displaying ship and outfit attributes.
	static std::string Number(double value);
	// Format the given value as a number with exactly the given number of
	// decimal places (even if they are all 0).
	static std::string Decimal(double value, int places);
	// Convert numbers to word forms. Capitalize the first letter if at the start of a sentence.
	static std::string WordForm(int64_t value, bool startOfSentence = false);
	// Conditionally convert numbers to word forms, based on the Chicago Manual of Style.
	static std::string ChicagoForm(int64_t value, bool startOfSentence = false);
	// Conditionally convert numbers to word forms, based on the MLA Style guide.
	static std::string MLAForm(int64_t value, bool startOfSentence = false);
	// Convert a string into a number. As with the output of Number(), the
	// string can have suffixes like "M", "B", etc.
	static double Parse(const std::string &str);
	// Replace a set of "keys," which must be strings in the form "<name>", with
	// a new set of strings, and return the result.
	static std::string Replace(const std::string &source, const std::map<std::string, std::string> &keys);
	// Recursively expand substitutions in all key/value pairs. Will detect
	// infinite recursion; offending substitutions will not be expanded.
	static void Expand(std::map<std::string, std::string> &keys);
	// Replace all occurrences of "target" with "replacement" in-place.
	static void ReplaceAll(std::string &text, const std::string &target, const std::string &replacement);

	// Convert a string to title caps or to lower case.
	static std::string Capitalize(const std::string &str);
	static std::string LowerCase(const std::string &str);

	// Split a single string into substrings with the given separator.
	static std::vector<std::string> Split(const std::string &str, const std::string &separator);

	// Finds &[condition] and &[format@condition] in strings and expands them
	static std::string ExpandConditions(const std::string &source, const ConditionGetter &getter);

	// Function for the "find" dialogs:
	static int Search(const std::string &str, const std::string &sub);
};



#endif
