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

using namespace std;



CompareStringsByGivenOrder::CompareStringsByGivenOrder(const vector<string>& order_)
	: order(order_)
{}



bool CompareStringsByGivenOrder::operator()(const string &a, const string &b) const
{
	if(a == b)
		return false;

	// Whichever is first in the array is considered smaller.
	for(const auto &it : order)
		if(it == a)
			return true;
		else if(it == b)
			return false;

	// Neither a nor b is a known value. Fall back to lexical comparison.
	return (a < b);
}
