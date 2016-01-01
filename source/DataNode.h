/* DataNode.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DATA_NODE_H_
#define DATA_NODE_H_

#include <list>
#include <string>
#include <vector>



// A DataNode is a single line of a DataFile. It consists of one or more tokens,
// which can be interpreted either as strings or as floating point values, and
// it may also have "children," which may each in turn have their own children.
// The tokens of a node are separated by white space, with quotation marks being
// used to group multiple words into a single token. If the token text contains
// quotation marks, it should be enclosed in backticks instead.
class DataNode {
public:
	DataNode(const DataNode *parent = nullptr);
	DataNode(const DataNode &other);
	
	DataNode &operator=(const DataNode &other);
	
	int Size() const;
	const std::string &Token(int index) const;
	double Value(int index) const;
	
	bool HasChildren() const;
	std::list<DataNode>::const_iterator begin() const;
	std::list<DataNode>::const_iterator end() const;
	
	// Print a message followed by a "trace" of this node and its parents.
	int PrintTrace(const std::string &message = "") const;
	
	
private:
	std::list<DataNode> children;
	std::vector<std::string> tokens;
	const DataNode *parent = nullptr;
	
	friend class DataFile;
};



#endif
