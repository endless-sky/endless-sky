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

#include <map>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class Sprite;



// Class representing a conversation, generally occurring when the you are asked to
// accept or decline a mission. The conversation can take different paths depending
// on what responses you choose, leading you to accept, decline, or (rarely) to be
// killed.
class Conversation {
public:
	// The possible outcomes of a conversation:
	static const int ACCEPT = -1;
	static const int DECLINE = -2;
	static const int DIE = -3;
	
	
public:
	Conversation();
	
	void Load(const DataNode &node);
	
	// The beginning of the conversation is node 0. Some nodes have choices for
	// the user to select; others just automatically continue to another node.
	bool IsChoice(int node) const;
	int Choices(int node) const;
	const std::string &Text(int node, int choice = 0) const;
	int NextNode(int node, int choice = 0) const;
	
	const Sprite *Scene() const;
	
	
private:
	class Node {
	public:
		Node(bool isChoice = false);
		
		std::vector<std::pair<std::string, int>> data;
		bool isChoice;
		bool canMergeOnto;
	};
	
	
private:
	// Add a label, pointing to whatever node is created next.
	void AddLabel(const std::string &label);
	// Set up a "goto". Depending on whether the named label has been seen yet
	// or not, it is either resolved immediately or added to the unresolved set.
	void Goto(const std::string &label, int node, int choice = 0);
	// Get the index of the given special string. 0 means it is "goto", a number
	// less than 0 means it is an outcome, and 1 means no match.
	static int TokenIndex(const std::string &token);
	
	
private:
	std::string identifier;
	std::map<std::string, int> labels;
	std::multimap<std::string, std::pair<int, int>> unresolved;
	std::vector<Node> nodes;
	
	const Sprite *scene;
};



#endif
