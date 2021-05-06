/* Conversation.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONVERSATION_H_
#define CONVERSATION_H_

#include "ConditionSet.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class DataWriter;
class Sprite;



// Class representing a conversation, generally occurring when the you are asked to
// accept or decline a mission. The conversation can take different paths depending
// on what responses you choose, leading you to accept, decline, or (rarely) to be
// killed. A conversation can also branch based on various condition flags that
// are set for the player, and can also modify those flags.
class Conversation {
public:
	// The possible outcomes of a conversation:
	static const int ACCEPT = -1;
	static const int DECLINE = -2;
	static const int DEFER = -3;
	// These 3 options force the player to TakeOff (if landed), or cause
	// the boarded NPCs to explode, in addition to respectively duplicating
	// the above mission outcomes.
	static const int LAUNCH = -4;
	static const int FLEE = -5;
	static const int DEPART = -6;
	// The player may simply die (if landed on a planet or captured while
	// in space), or the flagship might also explode.
	static const int DIE = -7;
	static const int EXPLODE = -8;
	
	// Check whether the given conversation outcome is one that forces the
	// player to immediately depart.
	static bool RequiresLaunch(int outcome);
	
public:
	// Read or write to files.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;
	// Check if any data is loaded in this conversation object.
	bool IsEmpty() const noexcept;
	// Check if this conversation includes a name prompt.
	bool IsValidIntro() const noexcept;
	
	// Do text replacement throughout this conversation. This returns a new
	// Conversation object with things like the player's name filled in.
	Conversation Substitute(const std::map<std::string, std::string> &subs) const;
	
	// The beginning of the conversation is node 0. Some nodes have choices for
	// the user to select; others just automatically continue to another node.
	// Nodes may also display images or include conditional branches.
	bool IsChoice(int node) const;
	int Choices(int node) const;
	bool IsBranch(int node) const;
	bool IsApply(int node) const;
	const ConditionSet &Conditions(int node) const;
	const std::string &Text(int node, int choice = 0) const;
	const Sprite *Scene(int node) const;
	int NextNode(int node, int choice = 0) const;
	
	
private:
	// The conversation is a network of "nodes" that you travel between by
	// making choices (or by automatic branches that depend on the condition
	// variable values for the current player).
	class Node {
	public:
		// Construct a new node. Each paragraph of conversation that involves no
		// choice can be merged into what came before it, to simplify things.
		explicit Node(bool isChoice = false) : isChoice(isChoice), canMergeOnto(!isChoice) {}
		
		// For applying condition changes or branching based on conditions:
		ConditionSet conditions;
		// The actual conversation text. If this node is not a choice, there
		// will only be one entry in the vector. Each entry also stores the
		// number of the node to go to next.
		std::vector<std::pair<std::string, int>> data;
		// A "choice" node can have only one option, so rather than checking if
		// data.size() != 1 we must explicitly store whether this is a choice:
		bool isChoice;
		// Keep track of whether it's possible to merge future nodes onto this.
		bool canMergeOnto;
		
		// Image that should be shown along with this text.
		const Sprite *scene = nullptr;
	};
	
	
private:
	// Parse the children of the given node to see if then contain any "gotos."
	// If so, link them up properly. Return true if gotos were found.
	bool LoadGotos(const DataNode &node);
	// Add a label, pointing to whatever node is created next.
	void AddLabel(const std::string &label, const DataNode &node);
	// Set up a "goto". Depending on whether the named label has been seen yet
	// or not, it is either resolved immediately or added to the unresolved set.
	void Goto(const std::string &label, int node, int choice = 0);
	// Add an "empty" node. It will contain one empty line of text, with its
	// goto link set to fall through to the next node.
	void AddNode();
	
	
private:
	// While parsing the conversation, keep track of what labels link to what
	// nodes. If a name appears in a goto before that label appears, remember
	// what node and what choice it appeared at in order to link it up later.
	std::map<std::string, int> labels;
	std::multimap<std::string, std::pair<int, int>> unresolved;
	// The actual conversation data:
	std::vector<Node> nodes;
};



#endif
