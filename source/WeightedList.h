/* WeightedList.h
Copyright (c) 2021 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WEIGHTED_LIST_H_
#define WEIGHTED_LIST_H_

#include "Random.h"

#include <cstddef>
#include <stdexcept>
#include <vector>

template <class Type>
using iterator = typename std::vector<Type>::iterator;
template <class Type>
using const_iterator = typename std::vector<Type>::const_iterator;



// Template representing a list of objects of a given type where each item in the
// list is weighted with an integer that is accessed via a Weight() function. This
// list can be queried to randomly return one object from the list where the
// probability of an object being returned is the weight of the object over the
// sum of the weights of all objects in the list.
template <class Type>
class WeightedList {
public:
	const Type &Get() const;
	std::size_t TotalWeight() const { return total; }
	
	iterator<Type> begin() noexcept { return choices.begin(); }
	const_iterator<Type> begin() const noexcept { return choices.begin(); }
	iterator<Type> end() noexcept { return choices.end(); }
	const_iterator<Type> end() const noexcept { return choices.end(); }
	
	void clear() noexcept { choices.clear(); total = 0; }
	std::size_t size() const noexcept { return choices.size(); }
	bool empty() const noexcept { return choices.empty(); }
	Type &back() noexcept { return choices.back(); }
	const Type &back() const noexcept { return choices.back(); }
	
	template <class ...Args>
	Type &emplace_back(Args&&... args);
	
	iterator<Type> eraseAt(iterator<Type> position) noexcept;
	iterator<Type> erase(iterator<Type> first, iterator<Type> last) noexcept;
	
	
private:
	std::vector<Type> choices;
	std::size_t total = 0;
};



template <class Type>
const Type &WeightedList<Type>::Get() const
{
	if(empty())
		throw std::runtime_error("Attempted to call Get on an empty weighted list.");
	
	unsigned index = 0;
	for(int choice = Random::Int(total); choice >= choices[index].Weight(); ++index)
		choice -= choices[index].Weight();
	
	return choices[index];
}



template <class Type>
template <class ...Args>
Type &WeightedList<Type>::emplace_back(Args&&... args)
{
	// Type is responsible for all weights being >= 1.
	choices.emplace_back(args...);
	Type &choice = choices.back();
	if(choice.Weight() < 1)
	{
		choices.pop_back();
		throw std::invalid_argument("Invalid weight inserted into weighted list. Weights must be >= 1.");
	}
	total += choice.Weight();
	return choice;
}



template <class Type>
iterator<Type> WeightedList<Type>::eraseAt(iterator<Type> position) noexcept
{
	total -= position->Weight();
	return choices.erase(position);
}



template <class Type>
iterator<Type> WeightedList<Type>::erase(iterator<Type> first, iterator<Type> last) noexcept
{
	for(auto it = first; it != last; ++it)
		total -= it->Weight();
	
	return choices.erase(first, last);
}



#endif
