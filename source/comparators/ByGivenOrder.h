/* ByGivenOrder.h
Copyright (c) 2022 by Michael Zahniser

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

#include <algorithm>
#include <vector>



/// Compare Ts according to the order specified at construction time.
/// Unknown Ts are considered larger than any known one.
template<class T>
class ByGivenOrder {
public:
	explicit ByGivenOrder(const std::vector<T>& order_)
		: order(order_)
	{}

	bool operator()(const T &a, const T &b) const
	{
		const auto find_a = std::find(order.begin(), order.end(), a);
		const auto find_b = std::find(order.begin(), order.end(), b);

		if(find_a == order.end() && find_b == order.end())
		{
			/// Neither a nor b is a known value. Fall back to default comparison.
			///
			return (a < b);
		}
		else
		{
			/// Whichever is first in the array is considered smaller.
			///
			return (find_a < find_b);
		}
	}


private:
	const std::vector<T>& order;
};
