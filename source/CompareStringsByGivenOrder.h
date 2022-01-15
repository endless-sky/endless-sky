/* CompareStringsByGivenOrder.h
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef COMPARE_STRINGS_BY_GIVEN_ORDER_H_
#define COMPARE_STRINGS_BY_GIVEN_ORDER_H_

#include <string>
#include <vector>



// Compare strings according to the order specified at construction time.
// Unknown strings are considered larger than any known string.
class CompareStringsByGivenOrder {
public:
	explicit CompareStringsByGivenOrder(const std::vector<std::string>& order_);

	bool operator()(const std::string &a, const std::string &b) const;

private:
	const std::vector<std::string>& order;
};



#endif
