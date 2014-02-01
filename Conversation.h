/* Conversation.h
Michael Zahniser, 9 Jan 2014

Class representing a conversation, generally occurring when the you are asked to
accept or decline a mission. The conversation can take different paths depending
on what responses you choose, leading you to accept, decline, or (rarely) to be
killed.
*/

#ifndef CONVERSATION_H_INCLUDED
#define CONVERSATION_H_INCLUDED

#include "DataFile.h"

#include <map>
#include <string>
#include <utility>
#include <vector>



class Conversation {
public:
	// The possible outcomes of a conversation:
	static const int ACCEPT = -1;
	static const int DECLINE = -2;
	static const int DIE = -3;
	
	
public:
	void Load(const DataFile::Node &node);
	
	// The beginning of the conversation is node 0. Some nodes have choices for
	// the user to select; others just automatically continue to another node.
	int Choices(int node) const;
	const std::string &Text(int node, int choice = 0) const;
	int NextNode(int node, int choice = 0) const;
	
	
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
};



#endif
