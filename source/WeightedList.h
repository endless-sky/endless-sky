/* Set.h
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

#include <vector>



// Template representing a list of objects of a given type where each item in the
// list is weighted with an integer. This list can be queried to randomly return
// one object from the list with the weight an object over the sum of the weights
// of all the objects in the list being the probability of it being returned.
template<class Type>
class WeightedList {
public:
	const Type &Get() const;
	int TotalWeight() const { return total; };
	
	typename std::vector<Type>::iterator begin() { return choices.begin(); }
	typename std::vector<Type>::const_iterator begin() const { return choices.begin(); }
	typename std::vector<Type>::iterator end() { return choices.end(); }
	typename std::vector<Type>::const_iterator end() const { return choices.end(); }
	
	void clear() { choices.clear(); };
	int size() const { return choices.size(); };
	bool empty() const { return choices.empty(); };
	Type &back() const { return choices.back(); };
	
	template <class ...Args>
	void emplace_back(Args&&... args);
	
	
private:
	mutable std::vector<Type> choices;
	int total = 0;
};



template <class Type>
template <class ...Args>
void WeightedList<Type>::emplace_back(Args&&... args)
{
	// Type is responsible for all weights being >= 1.
	choices.emplace_back(args...);
	total += choices.back().weight;
}



template<class Type>
const Type &WeightedList<Type>::Get() const
{
	unsigned index = 0;
	for(int choice = Random::Int(total); choice >= choices[index].weight; ++index)
		choice -= choices[index].weight;
	
	return choices[index];
}



#endif
