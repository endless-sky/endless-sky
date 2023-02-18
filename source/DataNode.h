/* DataNode.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DATA_NODE_H_
#define DATA_NODE_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "Condition.h"


class ConditionsStore;



// A DataNode is a single line of a DataFile. It consists of one or more tokens,
// which can be interpreted either as strings or as floating point values, and
// it may also have "children," which may each in turn have their own children.
// The tokens of a node are separated by white space, with quotation marks being
// used to group multiple words into a single token. If the token text contains
// quotation marks, it should be enclosed in backticks instead.
class DataNode {
public:
	static const char CONDITION_CHAR = '&';
	static constexpr const char *const CONDITION_CHAR_NAME = "an ampersand (\"&\")";


public:
	// Construct a DataNode. For the purpose of printing stack traces, each node
	// must remember what its parent node is.
	explicit DataNode(const DataNode *parent = nullptr) noexcept(false);
	// Copying or moving a DataNode requires updating the parent pointers.
	DataNode(const DataNode &other);
	DataNode &operator=(const DataNode &other);
	DataNode(std::shared_ptr<ConditionsStore>);
	DataNode(DataNode &&) noexcept;
	DataNode &operator=(DataNode &&) noexcept;

	// Get the number of tokens in this node.
	int Size() const noexcept;
	// Get all the tokens in this node as an iterable vector.
	const std::vector<std::string> &Tokens() const noexcept;
	// Get the token at the given index. No bounds checking is done internally.
	// DataFile loading guarantees index 0 always exists.
	const std::string &Token(int index) const;
	// Convert the token at the given index to a number. This returns 0 and prints an
	// error if the index is out of range or the token cannot be interpreted as a number.
	double Value(int index) const;
	static double Value(const std::string &token);
	// Check if the token at the given index is a number in a format that this
	// class is able to parse.
	bool IsNumber(int index) const;
	static bool IsNumber(const std::string &token);
	// Check if the token at the given index is in the format accepted for a
	// condition name
	bool IsCondition(int index) const;
	static bool IsCondition(const std::string &token);
	// Convert the token at the given index to a boolean. This returns false
	// and prints an error if the index is out of range or the token cannot
	// be interpreted as a number.
	bool BoolValue(int index) const;
	// Check if the token at the given index is a boolean, i.e. "true"/"1" or "false"/"0"
	// as a string.
	bool IsBool(int index) const;
	static bool IsBool(const std::string &token);

	// Check if this node has any children. If so, the iterator functions below
	// can be used to access them.
	bool HasChildren() const noexcept;
	std::list<DataNode>::const_iterator begin() const noexcept;
	std::list<DataNode>::const_iterator end() const noexcept;

	// Print a message followed by a "trace" of this node and its parents.
	int PrintTrace(const std::string &message = "") const;

	// Generates a condition with either the number at that index
	// or the condition variable at that index. The initial value of
	// the condition will be whatever is in the Store()
	Condition<double> AsCondition(int index) const;

	std::shared_ptr<ConditionsStore> Store();
	void SetStore(std::shared_ptr<ConditionsStore> store);

private:
	// Adjust the parent pointers when a copy is made of a DataNode.
	void Reparent() noexcept;


private:
	// These are "child" nodes found on subsequent lines with deeper indentation.
	std::list<DataNode> children;
	// These are the tokens found in this particular line of the data file.
	std::vector<std::string> tokens;
	// The parent pointer is used only for printing stack traces.
	const DataNode *parent = nullptr;
	// The line number in the given file that produced this node.
	size_t lineNumber = 0;

	// Used to generate a Condition:
	std::shared_ptr<ConditionsStore> store;

	// Allow DataFile to modify the internal structure of DataNodes.
	friend class DataFile;
};



#endif
