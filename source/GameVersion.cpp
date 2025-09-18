/* GameVersion.cpp
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GameVersion.h"

#include "text/Format.h"

#include <vector>

using namespace std;



strong_ordering GameVersion::operator<=>(const GameVersion &other) const
{
	strong_ordering numbersComparison = numbers <=> other.numbers;
	if(numbersComparison != 0)
		return numbersComparison;
	return fullRelease <=> other.fullRelease;
}



bool GameVersion::operator==(const GameVersion &other) const
{
	return numbers == other.numbers && fullRelease == other.fullRelease;
}



GameVersion::GameVersion(const string &versionString)
{
	auto alpha = versionString.find("-alpha");
	fullRelease = alpha == string::npos;

	vector<string> components = Format::Split(versionString.substr(0, alpha), ".");
	if(components.empty() || components.size() > 4)
		return;

	for(unsigned i = 0; i < components.size(); ++i)
	{
		unsigned number = 0;
		try {
			number = stoi(components[i]);
		}
		catch(const exception &)
		{
			return;
		}
		numbers[i] = number;
	}
	isValid = true;
}



string GameVersion::ToString() const
{
	return to_string(numbers[0]) + '.'
		+ to_string(numbers[1]) + '.'
		+ to_string(numbers[2]) + '.'
		+ to_string(numbers[3])
		+ (fullRelease ? "" : "-alpha");
}



bool GameVersion::IsValid() const
{
	return isValid;
}
