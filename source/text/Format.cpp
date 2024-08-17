/* Format.cpp
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

#include "Format.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
#include <sstream>
#include <unordered_set>

using namespace std;

namespace {
	const int64_t K = 1000;
	static const vector<pair<const char *, int64_t>> WORD_NUMBERS = {
		{ "quintillion", K * K * K * K * K * K },
		{ "quadrillion", K * K * K * K * K },
		{ "trillion", K * K * K * K },
		{ "billion", K * K * K },
		{ "million", K * K },
		{ "thousand", K }
	};
	static const vector<const char *> ONES_NAMES = {
		"zero ", "one ", "two ", "three ", "four ", "five ",
		"six ", "seven ", "eight ", "nine ", "ten ", "eleven ",
		"twelve ", "thirteen ", "fourteen ", "fifteen ",
		"sixteen ", "seventeen ", "eighteen ", "nineteen "
	};
	static const vector<const char *> TENS_NAMES = {
		"error", "error", "twenty", "thirty", "forty",
		"fifty", "sixty", "seventy", "eighty", "ninety"
	};

	// This struct exists just to allow the operator<< below.
	struct Wrapped {
		int64_t value;
	};

	// Implementation of WordForm. Outputs the word form of the wrapped number,
	// followed by a space.
	ostream &operator<< (ostream &o, const Wrapped &num)
	{

		if(num.value < 0)
			return o << "negative " << Wrapped { -num.value };

		Wrapped remaining { num };

		if(remaining.value >= 1000)
			for(auto &nameValue : WORD_NUMBERS)
				if(remaining.value >= nameValue.second)
				{
					Wrapped above { remaining.value / nameValue.second };
					remaining.value %= nameValue.second;
					o << above << nameValue.first;
					if(!remaining.value)
						return o;
					o << ' ';
				}

		if(remaining.value >= 100)
		{
			o << ONES_NAMES[(remaining.value / 100) % 10] << "hundred ";
			remaining.value %= 100;
			if(!remaining.value)
				return o;
		}

		if(remaining.value < 20)
			return o << ONES_NAMES[remaining.value];

		o << TENS_NAMES[remaining.value / 10];
		int64_t ones = remaining.value % 10;
		if(ones)
			return o << '-' << ONES_NAMES[ones];
		return o << ' ';
	}

	string MLAShorthand(int64_t value)
	{
		bool negative = value < 0;
		if(negative)
			value = -value;
		for(size_t magnitude = 0; magnitude < WORD_NUMBERS.size() - 1; ++magnitude)
		{
			int64_t above = value / WORD_NUMBERS[magnitude + 1].second;
			int64_t below = value % WORD_NUMBERS[magnitude + 1].second;
			if(above < 1000)
				continue;
			if(above >= 1000000 || !(above % 1000))
				break;
			if(below)
				continue;
			const size_t BUFLEN = 100;
			char buf[BUFLEN] = { 0 };
			snprintf(buf, BUFLEN, "%s%.3f %s",
				negative ? "negative " : "", above / 1000.0, WORD_NUMBERS[magnitude].first);
			buf[BUFLEN - 1] = '\0';
			return string(buf);
		}
		return string();
	}

	// Format an integer value, inserting its digits into the given string in
	// reverse order and then reversing the string.
	void FormatInteger(int64_t value, bool isNegative, string &result)
	{
		int places = 0;
		do {
			if(places && !(places % 3))
				result += ',';
			++places;

			result += static_cast<char>('0' + value % 10);
			value /= 10;
		} while(value);

		if(isNegative)
			result += '-';

		reverse(result.begin(), result.end());
	}

	string StringSubstituter(const string &source,
			function<const string *(const string &)> SubstitutionFor)
	{
		string target;
		target.reserve(source.length());

		string key;
		size_t start = 0;
		size_t search = start;
		while(search < source.length())
		{
			size_t left = source.find('<', search);
			if(left == string::npos)
				break;

			size_t right = source.find('>', left);
			if(right == string::npos)
				break;

			++right;
			size_t length = right - left;
			key.assign(source, left, length);
			const string *sub = SubstitutionFor(key);
			if(sub)
			{
				target.append(source, start, left - start);
				target.append(*sub, 0, string::npos);
				start = right;
				search = start;
			}
			else
				search = left + 1;
		}

		target.append(source, start, source.length() - start);
		return target;
	}

	// Helper function for Format::Expand, to recursively expand one key,
	// detecting cycles in the graph (and thus avoiding infinite recursion).
	void ExpandInto(const string &key, const string &oldValue, const map<string, string> &source,
			map<string, string> &result, unordered_set<string> &keysBeingExpanded)
	{
		// Optimization for a common case: no substitutions in the substitution.
		if(oldValue.find('<') == string::npos)
		{
			result.emplace(key, oldValue);
			return;
		}

		// Declare our intention to process this key so a later attempt will
		// detect recursion.
		auto inserted = keysBeingExpanded.insert(key);

		auto SubstitutionFor = [&](const string &request) -> const string *
		{
			auto hasResult = result.find(request);
			// Already finished this one.
			if(hasResult != result.end())
				return &hasResult->second;
			// Refuse to traverse a cycle in the graph.
			if(keysBeingExpanded.find(request) != keysBeingExpanded.end())
				return nullptr;
			auto hasSource = source.find(request);
			// Undefined key.
			if(hasSource == source.end())
				return nullptr;
			// This key-value pair has not been expanded yet.
			ExpandInto(request, hasSource->second, source, result, keysBeingExpanded);
			hasResult = result.find(request);
			return hasResult == result.end() ? nullptr : &hasResult->second;
		};

		string newValue = StringSubstituter(oldValue, SubstitutionFor);

		// Success! Indicate we're done expanding this key, and provide its value.
		keysBeingExpanded.erase(inserted.first);
		result.emplace(key, newValue);
	}

	// Helper function for ExpandConditions.
	//
	// source.substr(formatStart, formatSize) contains the format (credits, mass, etc.)
	// source.substr(conditionStart, conditionSize) contains the condition name
	//
	// If formatStart or formatSize are string::npos, then there is no formatting.
	//
	// The getter() acts like ConditionsStore.Get(), providing condition values.
	// These are passed through Format::Whatever(), and appended to the result.
	void AppendCondition(string &result, const string &source, const Format::ConditionGetter &getter,
		size_t formatStart, size_t formatSize, size_t conditionStart, size_t conditionSize)
	{
		int64_t value = getter(source, conditionStart, conditionSize);

		auto IsFormat = [&source, formatStart, formatSize](const char *format)
		{
			return !source.compare(formatStart, formatSize, format);
		};

		if(formatStart == string::npos || formatSize == string::npos)
			result.append(Format::Number(value));
		else if(IsFormat("raw"))
			result.append(to_string(value));
		else if(IsFormat("credits"))
			result.append(Format::CreditString(value)); // 1 credit, 2 credits, etc.
		else if(IsFormat("scaled"))
			result.append(Format::Credits(value)); // 35, 35k, 35M, etc.
		else if(IsFormat("tons"))
			result.append(Format::MassString(value)); // X tons or X ton
		else if(IsFormat("playtime"))
			result.append(Format::PlayTime(value)); // 3d 19h 24m 8s
		else if(IsFormat("chicago"))
			result.append(Format::ChicagoForm(value, false)); // thirty-three or 101
		else if(IsFormat("Chicago"))
			result.append(Format::ChicagoForm(value, true)); // Thirty-three or One hundred one
		else if(IsFormat("mla"))
			result.append(Format::MLAForm(value, false)); // thirty-three or 101
		else if(IsFormat("Mla"))
			result.append(Format::MLAForm(value, true)); // Thirty-three or One hundred one
		else if(IsFormat("words"))
			result.append(Format::WordForm(value, false)); // thirty-three or one hundred one
		else if(IsFormat("Words"))
			result.append(Format::WordForm(value, true)); // Thirty-three or One hundred one
		else
			// "number" or unsupported format
			result.append(Format::Number(value));
	}
}



// Convert the given number into abbreviated format with a suffix like
// "M" for million, "B" for billion, or "T" for trillion. Any number
// above 1 quadrillion is instead shown in scientific notation.
string Format::Credits(int64_t value)
{
	bool isNegative = (value < 0);
	int64_t absolute = abs(value);

	// If the value is above one quadrillion, show it in scientific notation.
	if(absolute > 1000000000000000ll)
	{
		ostringstream out;
		out.precision(3);
		out << static_cast<double>(value);
		return out.str();
	}

	// Reserve enough space for something like "-123.456M".
	string result;
	result.reserve(8);

	// Handle numbers bigger than a million.
	static const vector<char> SUFFIX = {'T', 'B', 'M'};
	static const vector<int64_t> THRESHOLD = {1000000000000ll, 1000000000ll, 1000000ll};
	for(size_t i = 0; i < SUFFIX.size(); ++i)
		if(absolute > THRESHOLD[i])
		{
			result += SUFFIX[i];
			int decimals = (absolute / (THRESHOLD[i] / 1000)) % 1000;
			for(int d = 0; d < 3; ++d)
			{
				result += static_cast<char>('0' + decimals % 10);
				decimals /= 10;
			}
			result += '.';
			absolute /= THRESHOLD[i];
			break;
		}

	// Convert the number to a string, adding commas if needed.
	FormatInteger(absolute, isNegative, result);
	return result;
}



// Convert the given number into abbreviated format as described in Format::Credits,
// then attach the ' credit' or ' credits' suffix to it.
string Format::CreditString(int64_t value)
{
	if(value == 1)
		return "1 credit";
	else
		return Credits(value) + " credits";
}



// Writes the given number into a string,
// then attach the ' ton' or ' tons' suffix to it.
string Format::MassString(double amount)
{
	if(amount == 1)
		return "1 ton";
	return Format::Number(amount) + " tons";
}



// Creates a string similar to '<amount> tons of <cargo>'.
string Format::CargoString(double amount, const string &cargo)
{
	return MassString(amount) + " of " + cargo;
}



// Convert a time in seconds to years/days/hours/minutes/seconds
string Format::PlayTime(double timeVal)
{
	string result;
	int timeValFormat = 0;
	static const array<char, 5> SUFFIX = {'s', 'm', 'h', 'd', 'y'};
	static const array<int, 4> PERIOD = {60, 60, 24, 365};

	timeValFormat = max(0., timeVal);
	// Break time into larger and larger units until the largest one, or the value is empty
	size_t i = 0;
	do {
		int period = (i < SUFFIX.size() - 1 ? timeValFormat % PERIOD[i] : timeValFormat);
		result = (i == 0 ? result + SUFFIX[i] : result + ' ' + SUFFIX[i]);
		do {
			result += static_cast<char>('0' + period % 10);
			period /= 10;
		} while(period);
		if(i < PERIOD.size())
			timeValFormat /= PERIOD[i];
		i++;
	} while (timeValFormat && i < SUFFIX.size());

	reverse(result.begin(), result.end());
	return result;
}



// Convert the given number to a string, with a reasonable number of decimal
// places. (This is primarily for displaying ship and outfit attributes.)
string Format::Number(double value)
{
	if(!value)
		return "0";
	else if(std::isnan(value))
		return "???";
	else if(std::isinf(value))
		return value > 0. ? "infinity" : "-infinity";

	string result;
	bool isNegative = (value < 0.);
	value = fabs(value);

	// Only show decimal places for numbers between +/-10'000.
	double decimal = modf(value, &value);
	if(decimal && value < 10000)
	{
		double tenths = 0.;
		// Account for floating-point representation error by adding EPS after multiplying.
		constexpr double EPS = 0.0000000001;
		int hundredths = static_cast<int>(EPS + 10. * modf(decimal * 10., &tenths));
		if(hundredths > 9)
		{
			hundredths = 0;
			++tenths;
		}
		if(tenths >= 10. - EPS)
		{
			++value;
			tenths = hundredths = 0;
		}

		// Values up to 1000 may have two decimal places.
		bool two = value < 1000 && hundredths;
		if(two)
			result += static_cast<char>('0' + hundredths);
		if(two || tenths)
		{
			result += static_cast<char>('0' + tenths);
			result += '.';
		}
	}

	// Convert the number to a string, adding commas if needed.
	FormatInteger(value, isNegative, result);
	return result;
}



// Format the given value as a number with exactly the given number of
// decimal places (even if they are all 0).
string Format::Decimal(double value, int places)
{
	double integer;
	double fraction = fabs(modf(value, &integer));

	string result = to_string(static_cast<int>(integer)) + ".";
	while(places--)
	{
		fraction = modf(fraction * 10., &integer);
		result += ('0' + static_cast<int>(integer));
	}
	return result;
}



string Format::WordForm(int64_t value, bool startOfSentence)
{
	ostringstream o;
	o << Wrapped { value };
	string result = o.str();
	if(result.size() > 0 && result[result.size() - 1] == ' ')
		result.resize(result.size() - 1);
	if(!result.empty() && startOfSentence && result[0] >= 'a' && result[0] <= 'z')
		result[0] -= 32;
	return result;
}



// Chicago manual of style
string Format::ChicagoForm(int64_t value, bool startOfSentence)
{
	if(startOfSentence)
		return WordForm(value, true);
	if(value < 1000 && value > -1000 && ! (value % 100))
		return WordForm(value, startOfSentence);
	int64_t above = value, below = 0;
	for(int i = 0; above && i < 6; i++)
	{
		if(below)
			break;
		else if(above < 100 && above > -100)
			return WordForm(value, startOfSentence);
		else if(above < 1000 && above > -1000 && !(above % 100))
			return WordForm(value, startOfSentence);
		below = above % 1000;
		above /= 1000;
	}
	return Format::Number(value);
}



// MLA Handbook style
string Format::MLAForm(int64_t value, bool startOfSentence)
{
	if(startOfSentence)
		return WordForm(value, true);
	if(value >= -99 && value <= 99)
		return WordForm(value, startOfSentence);

	// 21350000 => 21.35 million
	string shorthand = MLAShorthand(value);
	if(!shorthand.empty())
		return shorthand;

	int64_t above = value, below = 0;
	for(int i = 0; above && i < 6; i++)
	{
		if(below)
			break;
		else if(above <= 10 && above >= -10)
			return WordForm(value, startOfSentence);
		else if(above < 100 && above > -100 && !(above % 10))
			return WordForm(value, startOfSentence);
		below = above % 1000;
		above /= 1000;
	}
	return Format::Number(value);
}



// Convert a string into a number. As with the output of Number(), the
// string can have suffixes like "M", "B", etc.
// It can also contain spaces or "," as separators like 1,000 or 1 000.
// Does not support parsing NaN or infinite values.
double Format::Parse(const string &str)
{
	double place = 1.;
	double value = 0.;

	string::const_iterator it = str.begin();
	string::const_iterator end = str.end();
	while(it != end && (*it < '0' || *it > '9') && *it != '.')
		++it;

	for( ; it != end; ++it)
	{
		if(*it == '.')
			place = .1;
		else if(*it == ',' || *it == ' ') {}
		else if(*it < '0' || *it > '9')
			break;
		else
		{
			double digit = *it - '0';
			if(place < 1.)
			{
				value += digit * place;
				place *= .1;
			}
			else
			{
				value *= 10.;
				value += digit;
			}
		}
	}

	if(it != end)
	{
		if(*it == 'k' || *it == 'K')
			value *= 1e3;
		else if(*it == 'm' || *it == 'M')
			value *= 1e6;
		else if(*it == 'b' || *it == 'B')
			value *= 1e9;
		else if(*it == 't' || *it == 'T')
			value *= 1e12;
	}

	return value;
}



string Format::Replace(const string &source, const map<string, string> &keys)
{
	auto SubstitutionFor = [&](const string &key) -> const string *
	{
		auto found = keys.find(key);
		return (found == keys.end()) ? nullptr : &found->second;
	};

	return StringSubstituter(source, SubstitutionFor);
}



void Format::Expand(map<string, string> &keys)
{
	map<string, string> newKeys;
	unordered_set<string> keysBeingExpanded;
	for(auto it = keys.begin(); it != keys.end(); ++it)
		if(newKeys.find(it->first) == newKeys.end())
			ExpandInto(it->first, it->second, keys, newKeys, keysBeingExpanded);
	keys.swap(newKeys);
}



void Format::ReplaceAll(string &text, const string &target, const string &replacement)
{
	// If the searched string is an empty string, do nothing.
	if(target.empty())
		return;

	string newString;
	newString.reserve(text.length());

	// Index at which to begin searching for the target string.
	size_t start = 0;
	size_t matchLength = target.length();
	// Index at which the target string was found.
	size_t findPos = string::npos;
	while((findPos = text.find(target, start)) != string::npos)
	{
		newString.append(text, start, findPos - start);
		newString += replacement;
		start = findPos + matchLength;
	}

	// Add the remaining text.
	newString += text.substr(start);

	text.swap(newString);
}



string Format::Capitalize(const string &str)
{
	string result = str;
	bool first = true;
	for(char &c : result)
	{
		if(isspace(static_cast<unsigned char>(c)))
			first = true;
		else
		{
			if(first && islower(static_cast<unsigned char>(c)))
				c = toupper(static_cast<unsigned char>(c));
			first = false;
		}
	}
	return result;
}



string Format::LowerCase(const string &str)
{
	string result = str;
	for(char &c : result)
		c = tolower(static_cast<unsigned char>(c));
	return result;
}



// Split a single string into substrings with the given separator.
vector<string> Format::Split(const string &str, const string &separator)
{
	vector<string> result;
	size_t begin = 0;
	while(true)
	{
		size_t pos = str.find(separator, begin);
		if(pos == string::npos)
			pos = str.length();
		result.emplace_back(str, begin, pos - begin);
		begin = pos + separator.size();
		if(begin >= str.length())
			break;
	}
	return result;
}



string Format::ExpandConditions(const string &source, const ConditionGetter &getter)
{
	// Optimization for most common case: no conditions
	if(source.find('&') == string::npos)
		return source;

	string result;
	result.reserve(source.size());

	size_t formatStart = string::npos;
	size_t formatSize = string::npos;
	size_t conditionStart = string::npos;
	size_t conditionSize = string::npos;

	// Hand-coded regular grammar parser for:
	//	&[format@condition]
	//	&[condition]
	// Using these states:
	//	state = _ ----- outside of all &[] regions
	//	state = & ----- just read & and hoping to see a [
	//	state = [ ----- read &[ but haven't seen @ or ] yet
	//	state = N ----- inside a nested [] of depth `depth`
	//	state = @ ----- read &[...@ but haven't seen ] yet. Have format start & size.

	// Anything inside a &[...] is sent to AppendCondition

	static const char OUTER = '_';
	static const char PREFIX = '&';
	static const char LPAREN = '[';
	static const char RPAREN = ']';
	static const char DIVIDER = '@';
	static const char NESTED = 'N';

	char state = OUTER;
	// Depth of nested [] within the &[...]
	int depth = 0;
	// State before entering the nested []
	char oldState = LPAREN;
	// "start" is the beginning of the text that has not yet been sent to result.
	size_t start = 0;
	for(size_t look = 0; look < source.size(); ++look)
	{
		char next = source[look];
		// This would be faster with a nested select, but that would be
		// harder to read, and I don't expect this to be performance-critical.
		if(state == OUTER && next == PREFIX)
		{
			if(look > start)
			{
				result.append(source, start, look - start);
				start = look;
			}
			state = PREFIX;
		}
		else if(state == OUTER || (state == PREFIX && next != LPAREN))
			// Accumulate one character to print outside of any &[@]
			state = OUTER;
		else if(state == PREFIX && next == LPAREN)
		{
			formatStart = formatSize = conditionStart = conditionSize = string::npos;
			state = LPAREN;
		}
		else if(state == LPAREN && next == DIVIDER)
		{
			formatStart = start + 2;
			formatSize = look - formatStart;
			state = DIVIDER;
		}
		else if(state == DIVIDER && next == RPAREN)
		{
			conditionStart = formatStart + formatSize + 1;
			conditionSize = look - conditionStart;
			AppendCondition(result, source, getter, formatStart, formatSize,
				conditionStart, conditionSize);
			start = look + 1;
			state = OUTER;
		}
		else if((state == LPAREN || state == DIVIDER) && next == LPAREN)
		{
			oldState = state;
			state = NESTED;
			depth = 1;
		}
		else if(state == NESTED && next == LPAREN)
			depth++;
		else if(state == NESTED && next == RPAREN && depth > 1)
			depth--;
		else if(state == NESTED && next == RPAREN && depth == 1)
			state = oldState;
		else if(state == LPAREN && next == RPAREN)
		{
			conditionStart = start + 2;
			conditionSize = look - conditionStart;
			AppendCondition(result, source, getter, formatStart, formatSize,
				conditionStart, conditionSize);
			start = look + 1;
			state = OUTER;
		}
		else if(!(state == LPAREN || state == DIVIDER || state == NESTED))
		{
			// Error in format string.
			result.append(source, start, look - start + 1);
			start = look + 1;
			state = OUTER;
		}
	}
	if(start < source.size())
		result.append(source, start, string::npos);
	return result;
}



int Format::Search(const string &str, const string &sub)
{
	auto it = search(str.begin(), str.end(), sub.begin(), sub.end(),
		[](unsigned char a, unsigned char b) { return toupper(a) == toupper(b); });
	return (it == str.end() ? -1 : it - str.begin());
}
