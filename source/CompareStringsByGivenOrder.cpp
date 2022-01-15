/* CompareStringsByGivenOrder.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CompareStringsByGivenOrder.h"

#include <algorithm>

using namespace std;



CompareStringsByGivenOrder::CompareStringsByGivenOrder(const vector<string>& order_)
	: order(order_)
{}



bool CompareStringsByGivenOrder::operator()(const string &a, const string &b) const
{
	const auto find_a = find(order.begin(), order.end(), a);
	const auto find_b = find(order.begin(), order.end(), b);

	if(find_a == order.end() && find_b == order.end())
	{
		// Neither a nor b is a known value. Fall back to lexical comparison.
		return (a < b);
	}
	else
	{
		// Whichever is first in the array is considered smaller.
		return (find_a < find_b);
	}
}
