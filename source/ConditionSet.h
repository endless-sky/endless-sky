/* ConditionSet.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONDITION_SET_H_
#define CONDITION_SET_H_

#include <map>
#include <string>
#include <vector>

class DataNode;
class DataWriter;



// A condition set is a collection of operations on the player's set of named
// "conditions". This includes "test" operations that just check the values of
// those conditions, and other operations that can be "applied" to change the
// values.
class ConditionSet {
public:
	// Load a set of conditions from the children of this node.
	void Load(const DataNode &node);
	// Write a set of conditions, but at the current indentation level, because
	// they may be interspersed with other data.
	void Save(DataWriter &out) const;
	
	bool IsEmpty() const;
	
	// Read a single condition from a data node.
	void Add(const DataNode &node);
	bool Add(const std::string &firstToken, const std::string &secondToken);
	bool Add(const std::string &name, const std::string &op, int value);
	
	bool Test(const std::map<std::string, int> &conditions) const;
	void Apply(std::map<std::string, int> &conditions) const;
	
	
private:
	class Entry {
	public:
		Entry(const std::string &name, const std::string &op, int value);
		
		std::string name;
		// This needs to be saved for saving conditions.
		std::string op;
		int (*fun)(int, int);
		double value;
	};
	
	
private:
	bool isOr = false;
	std::vector<Entry> entries;
	std::vector<ConditionSet> children;
};



#endif
