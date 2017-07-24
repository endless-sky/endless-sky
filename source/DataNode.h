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

#include "SHA1.h"

#include <functional>
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
	// Construct a DataNode. For the purpose of printing stack traces, each node
	// must remember what its parent node is.
	explicit DataNode(const DataNode *parent = nullptr);
	// Copy constructor.
	DataNode(const DataNode &other);
	
	DataNode &operator=(const DataNode &other);
	
	// Get the number of tokens in this node.
	int Size() const;
	// Get the token at the given index. No bounds checking is done internally.
	const std::string &Token(int index) const;
	// Convert the token at the given index to a number. This returns 0 if the
	// index is out of range or the token cannot be interpreted as a number.
	double Value(int index) const;
	// Check if the token at the given index is a number in a format that this
	// class is able to parse.
	bool IsNumber(int index) const;
	
	// Check if this node has any children. If so, the iterator functions below
	// can be used to access them.
	bool HasChildren() const;
	std::list<DataNode>::const_iterator begin() const;
	std::list<DataNode>::const_iterator end() const;
	
	// Print a message followed by a "trace" of this node and its parents.
	int PrintTrace(const std::string &message = "") const;
	
	// Computes a hash of this DataNode and its descendants, if they match the
	// given predicate. If 'includeThis' is false, only descendants are hashed.
	// If the predicate is null, all nodes are hashed.
	std::string GetHash(bool includeThis, const std::function<bool(const DataNode &)> &predicate = nullptr) const;
	
	
private:
	// Recursively adds this node to a hash.
	void RecursivelyHash(SHA1 &sha, bool includeThis, const std::function<bool(const DataNode &)> &predicate) const;
	// Adjust the parent pointers when a copy is made of a DataNode.
	void Reparent();
	
	
private:
	// constant values used to compute the hash of a data file
	static const char * const HashSeed;
	static constexpr const char IndentHash = '\x01', DedentHash = '\x02';
	friend class DataWriter; // DataWriter needs these constants to compute the hash of output
	
	// These are "child" nodes found on subsequent lines with deeper indentation.
	std::list<DataNode> children;
	// These are the tokens found in this particular line of the data file.
	std::vector<std::string> tokens;
	// The parent pointer is used only for printing stack traces.
	const DataNode *parent = nullptr;
	
	// Allow DataFile to modify the internal structure of DataNodes.
	friend class DataFile;
};



#endif
