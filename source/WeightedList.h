/* WeightedList.h
Copyright (c) 2021 by Amazinite

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

#include "Random.h"

#include <cstddef>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>



// Template representing a list of objects of a given type where each item in the
// list is weighted with an integer. This list can be queried to randomly return
// one object from the list where the probability of an object being returned is
// the weight of the object over the sum of the weights of all objects in the list.
template<class Type>
class WeightedList {
	using iterator = typename std::vector<Type>::iterator;
	using const_iterator = typename std::vector<Type>::const_iterator;
public:
	template<class T>
	friend std::size_t erase(WeightedList<T> &list, const T &item);
	template<class T, class UnaryPredicate>
	friend std::size_t erase_if(WeightedList<T> &list, UnaryPredicate pred);

	const Type &Get() const;
	std::size_t TotalWeight() const noexcept { return total; }

	// Average the result of the given function by the choices' weights.
	template<class Callable>
	std::enable_if_t<
		std::is_arithmetic_v<std::invoke_result_t<Callable &&, const Type &&>>,
		// The return type of WeightedList::Average, if the above test passes:
		std::invoke_result_t<Callable &&, const Type &&>
	> Average(Callable c) const;
	// Supplying a callable that does not return an arithmetic value will fail to compile.

	iterator begin() noexcept { return choices.begin(); }
	const_iterator begin() const noexcept { return choices.begin(); }
	iterator end() noexcept { return choices.end(); }
	const_iterator end() const noexcept { return choices.end(); }

	void clear() noexcept { choices.clear(); weights.clear(); total = 0; }
	void reserve(std::size_t n) { choices.reserve(n); weights.reserve(n); }
	std::size_t size() const noexcept { return choices.size(); }
	bool empty() const noexcept { return choices.empty(); }
	Type &back() noexcept { return choices.back(); }
	const Type &back() const noexcept { return choices.back(); }

	template<class ...Args>
	Type &emplace_back(int weight, Args &&...args);

	iterator eraseAt(iterator position) noexcept;
	iterator erase(iterator first, iterator last) noexcept;


private:
	void RecalculateWeight();


private:
	std::vector<Type> choices;
	std::vector<std::size_t> weights;
	std::size_t total = 0;
};



template<class T>
std::size_t erase(WeightedList<T> &list, const T &item)
{
	return erase_if(list, [&item](const T &t) noexcept -> bool { return item == t; });
}



template<class T, class UnaryPredicate>
std::size_t erase_if(WeightedList<T> &list, UnaryPredicate pred)
{
	std::size_t erased = 0;
	unsigned available = list.choices.size() - 1;
	for(unsigned index = 0; index < list.choices.size() - erased; ++index)
		if(pred(list.choices[index]))
		{
			while(index != available && pred(list.choices[available]))
			{
				--available;
				++erased;
			}
			if(index != available)
			{
				list.choices[index] = std::move(list.choices[available]);
				list.weights[index] = std::move(list.weights[available]);
				--available;
			}
			++erased;
		}

	list.choices.erase(std::prev(list.choices.end(), erased), list.choices.end());
	list.weights.erase(std::prev(list.weights.end(), erased), list.weights.end());
	list.RecalculateWeight();

	return erased;
}



template<class Type>
const Type &WeightedList<Type>::Get() const
{
	if(empty())
		throw std::runtime_error("Attempted to call Get on an empty weighted list.");

	unsigned index = 0;
	for(unsigned choice = Random::Int(total); choice >= weights[index]; ++index)
		choice -= weights[index];

	return choices[index];
}



template<class Type>
template<class Callable>
std::enable_if_t<
	std::is_arithmetic_v<std::invoke_result_t<Callable &&, const Type &&>>,
	std::invoke_result_t<Callable &&, const Type &&>
> WeightedList<Type>::Average(Callable fn) const
{
	std::size_t tw = TotalWeight();
	if(tw == 0) return 0;

	auto sum = typename std::invoke_result_t<Callable, const Type &>{};
	for(unsigned index = 0; index < choices.size(); ++index)
		sum += fn(choices[index]) * weights[index];
	return sum / tw;
}



template<class Type>
template<class ...Args>
Type &WeightedList<Type>::emplace_back(int weight, Args &&...args)
{
	// All weights must be >= 1.
	if(weight < 1)
		throw std::invalid_argument("Invalid weight inserted into weighted list. Weights must be >= 1.");

	choices.emplace_back(std::forward<Args>(args)...);
	weights.emplace_back(weight);
	total += weights.back();
	return choices.back();
}



template<class Type>
typename std::vector<Type>::iterator WeightedList<Type>::eraseAt(typename std::vector<Type>::iterator position) noexcept
{
	unsigned index = std::distance(choices.begin(), position);
	total -= weights[index];
	weights.erase(std::next(weights.begin(), index));
	return choices.erase(position);
}



template<class Type>
typename std::vector<Type>::iterator WeightedList<Type>::erase(typename std::vector<Type>::iterator first,
	typename std::vector<Type>::iterator last) noexcept
{
	auto firstWeight = std::next(weights.begin(), std::distance(choices.begin(), first));
	auto lastWeight = std::next(weights.begin(), std::distance(choices.begin(), last));
	weights.erase(firstWeight, lastWeight);
	RecalculateWeight();
	return choices.erase(first, last);
}



template<class Type>
void WeightedList<Type>::RecalculateWeight()
{
	total = std::accumulate(weights.begin(), weights.end(), 0);
}
