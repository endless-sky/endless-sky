/* OrderedSet.h
Copyright (c) 2026 by xobes

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

#include "Set.h"

#include <algorithm>
#include <string>
#include <vector>



// From Set.h:
// Template representing a set of named objects of a given type, where you can
// query it for a pointer to any object and it will return one, whether or not that
// object has been loaded yet. (This allows cyclic pointers.)

// This variant maintains insertion order unless that order is manipulated thereafter.
template<class Type>
class OrderedSet : private Set<Type> {
public:
	using Set<Type>::Find;
	using Set<Type>::Has;
	using Set<Type>::size;
	using Set<Type>::empty;

	// This iterator class will allow us to produce a pair of string-value in a loop with
	// the order following that of the vector.
	class OrderedSetIterator {
	public:
		OrderedSetIterator(OrderedSet<Type> *dataset, std::vector<std::string>::iterator iter)
			: dataset(dataset), orderIter(iter) {}

		std::pair<const std::string, Type> operator*() const
		{
			return make_pair(*orderIter, *dataset->Get(*orderIter));
		}

		OrderedSetIterator &operator++()
		{
			++orderIter;
			return *this;
		}

		bool operator!=(const OrderedSetIterator &other) const
		{
			return orderIter != other.orderIter;
		}

	private:
		OrderedSet<Type> *dataset;
		std::vector<std::string>::iterator orderIter;
	};

	OrderedSetIterator begin()
	{
		return OrderedSetIterator(this, order.begin());
	}

	OrderedSetIterator end()
	{
		return OrderedSetIterator(this, order.end());
	}

	Type *Get(const std::string &name)
	{
		// Since Set::Get will create new instances of <Type> when `name` cannot be found,
		// this will tack them onto the end of the vector we are tracking items with.
		// The order can be changed afterward as needed.
		Type *retVal = Set<Type>::Get(name);
		if(std::find(order.begin(), order.end(), name) == order.end())
			order.push_back(name);
		return retVal;
	}

	const Type *Get(const std::string &name) const
	{
		return Set<Type>::Get(name);
	}

	void Remove(const std::string &name)
	{
		Set<Type>::Remove(name);
		auto it = std::find(order.begin(), order.end(), name);
		if(it != order.end())
			order.erase(it);
	}

	void Sort()
	{
		std::sort(order.begin(), order.end());
	}

	// TODO: provide mechanisms for moving elements of the vector around



private:
	mutable std::vector<std::string> order;
};
