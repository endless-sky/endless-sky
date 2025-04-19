/* ByPair.h
Copyright (c) 2025 by Michael Zahniser

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

#include <utility>



template<class T1, class T2>
class ByFirstInPair {
public:
	bool operator()(const std::pair<T1, T2> *a, const std::pair<T1, T2> *b) const
	{
		return a->first < b->first;
	}

	bool operator()(const std::pair<T1, T2> &a, const std::pair<T1, T2> &b) const
	{
		return a.first < b.first;
	}
};



template<class T1, class T2>
class BySecondInPair {
public:
	bool operator()(const std::pair<T1, T2> *a, const std::pair<T1, T2> *b) const
	{
		return a->second < b->second;
	}

	bool operator()(const std::pair<T1, T2> &a, const std::pair<T1, T2> &b) const
	{
		return a.second < b.second;
	}
};
