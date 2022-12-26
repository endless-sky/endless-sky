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

#ifndef WEIGHTED_LIST_H_
#define WEIGHTED_LIST_H_

#include "Random.h"
#include "Condition.h"

#include <cstddef>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>



// Weights must be at least zero, and must be finite.
template <class T>
static bool IsAValidWeight(const T &t) {
	return t >= 0 && (t + 1 > t);
}



// Template representing a list of objects of a given type where each item in the
// list is weighted with an integer. This list can be queried to randomly return
// one object from the list where the probability of an object being returned is
// the weight of the object over the sum of the weights of all objects in the list.
template <class Type, class WeightType = unsigned>
class WeightedList {
	using iterator = typename std::vector<Type>::iterator;
	using const_iterator = typename std::vector<Type>::const_iterator;
public:
	template <class T, class W>
	friend std::size_t erase(WeightedList<T, W> &list, const T &item);
	template <class T, class W, class UnaryPredicate>
	friend std::size_t erase_if(WeightedList<T, W> &list, UnaryPredicate pred);

	// Choose a random fleet based on weights. If all weights are 0, returns the first fleet.
	const Type &Get() const;
	std::size_t TotalWeight() const noexcept { return total; }

	// Average the result of the given function by the choices' weights.
	template <class Callable>
	typename std::enable_if<
		std::is_arithmetic<typename std::result_of<Callable&&(const Type&&)>::type>::value,
		// The return type of WeightedList::Average, if the above test passes:
		typename std::result_of<Callable&&(const Type&&)>::type
	>::type Average(Callable c) const;
	// Supplying a callable that does not return an arithmetic value will fail to compile.

	// Update weight values. This is to support the use of Condition weights.
	// The Getter must be a class with a HasGet method like ConditionsStore
	template <class Getter>
	void UpdateConditions(const Getter &c);

	// At least one choice has c(weight,choice) = true
	template <class Callable>
	bool Any(Callable c) const;

	// All choices have c(weight,choice) = true
	template <class Callable>
	bool All(Callable c) const;

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

	template <class NewWeightType, class ...Args>
	Type &emplace_back(const NewWeightType &weight, Args&&... args);

	iterator eraseAt(iterator position) noexcept;
	iterator erase(iterator first, iterator last) noexcept;


private:
	void RecalculateWeight();


private:
	std::vector<Type> choices;
	std::vector<WeightType> weights;
	std::size_t total = 0;
};



template <class T,class U>
void AssignWeight(T &t, U u)
{
	static_assert(std::is_integral<T>::value, "AssignWeight takes only integral types or Conditions");
	t = u;
}



template <class T,class U>
void AssignWeight(Condition<T> &t,U u)
{
	t.Value() = u;
}



template <class T,class W>
std::size_t erase(WeightedList<T,W> &list, const T &item)
{
	return erase_if(list, [&item](const T &t) noexcept -> bool { return item == t; });
}



template <class T, class W, class UnaryPredicate>
std::size_t erase_if(WeightedList<T,W> &list, UnaryPredicate pred)
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



template <class Type, class WeightType>
const Type &WeightedList<Type,WeightType>::Get() const
{
	if(empty())
		throw std::runtime_error("Attempted to call Get on an empty weighted list.");

	if(!total)
		// When no choices are enabled, return the first.
		return choices[0];

	unsigned choice0 = Random::Int(total);
	unsigned choice = choice0;
	for(unsigned index = 0; index < weights.size() ; ++index)
	{
		if(!weights[index])
			continue;
		else if(choice < weights[index])
			return choices[index];
		else
			choice -= weights[index];
	}

	// Failsafe. Should not get here.
	return choices[0];
}



template <class Type, class WeightType>
template <class Callable>
typename std::enable_if<
	std::is_arithmetic<typename std::result_of<Callable&&(const Type&&)>::type>::value,
	typename std::result_of<Callable&&(const Type&&)>::type
>::type WeightedList<Type,WeightType>::Average(Callable fn) const
{
	std::size_t tw = TotalWeight();
	if(tw == 0) return 0;

	auto sum = typename std::result_of<Callable(const Type &)>::type{};
	for(unsigned index = 0; index < choices.size(); ++index)
		sum += fn(choices[index]) * static_cast<unsigned>(weights[index]);
	return sum / tw;
}



template <class Type, class WeightType>
template <class NewWeightType, class ...Args>
Type &WeightedList<Type,WeightType>::emplace_back(const NewWeightType &newWeight, Args&&... args)
{
	WeightType weight = IsAValidWeight(newWeight) ? WeightType(newWeight) : WeightType(0);
	choices.emplace_back(std::forward<Args>(args)...);
	weights.emplace_back(weight);
	total += static_cast<std::size_t>(weights.back());
	return choices.back();
}



// Update weight values. This is to support the use of Condition weights.
// The Getter must be a class with a HasGet method like ConditionsStore
template <class Type, class WeightType>
template <class Getter>
void WeightedList<Type, WeightType>::UpdateConditions(const Getter &getter)
{
	for(unsigned index = 0; index < choices.size(); ++index)
	{
		weights[index].UpdateConditions(getter, [&](typename Getter::ValueType w)
		{
			return IsAValidWeight(w);
		});
		choices[index].UpdateConditions(getter);
	}
	RecalculateWeight();
}



// At least one has c(weight,choice) = true
template <class Type, class WeightType>
template <class Callable>
bool WeightedList<Type, WeightType>::Any(Callable c) const
{
	for(unsigned index = 0; index < choices.size(); ++index)
		if(c(weights[index], choices[index]))
			return true;
	return false;
}


// All choices have c(weight,choice) = true
template <class Type, class WeightType>
template <class Callable>
bool WeightedList<Type, WeightType>::All(Callable c) const
{
	for(unsigned index = 0; index < choices.size(); ++index)
		if(!c(weights[index], choices[index]))
			return false;
	return true;
}



template <class Type, class WeightType>
typename std::vector<Type>::iterator WeightedList<Type,WeightType>::eraseAt(
	typename std::vector<Type>::iterator position) noexcept
{
	unsigned index = std::distance(choices.begin(), position);
	total -= static_cast<std::size_t>(weights[index]);
	weights.erase(std::next(weights.begin(), index));
	return choices.erase(position);
}



template <class Type, class WeightType>
typename std::vector<Type>::iterator WeightedList<Type,WeightType>::erase(typename std::vector<Type>::iterator first,
	typename std::vector<Type>::iterator last) noexcept
{
	auto firstWeight = std::next(weights.begin(), std::distance(choices.begin(), first));
	auto lastWeight = std::next(weights.begin(), std::distance(choices.begin(), last));
	weights.erase(firstWeight, lastWeight);
	RecalculateWeight();
	return choices.erase(first, last);
}



template <class Type, class WeightType>
void WeightedList<Type,WeightType>::RecalculateWeight()
{
	total = 0;
	for(auto &weight : weights)
		total += static_cast<std::size_t>(weight);
}



#endif
