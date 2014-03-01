/* Conversation.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Conversation.h"

#include "SpriteSet.h"

#include <iostream>

using namespace std;



Conversation::Conversation()
	: scene(nullptr)
{
}



void Conversation::Load(const DataFile::Node &node)
{
	if(node.Token(0) != "conversation" || node.Size() < 2)
		return;
	identifier = node.Token(1);
	
	// Free any previously loaded data.
	nodes.clear();
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "scene" && child.Size() >= 2)
			scene = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "label" && child.Size() >= 2)
		{
			// You cannot merge text above a label with text below it.
			if(!nodes.empty())
				nodes.back().canMergeOnto = false;
			AddLabel(child.Token(1));
		}
		else if(child.Token(0) == "choice")
		{
			// Create a new node with one or more choices in it.
			nodes.emplace_back(true);
			for(const DataFile::Node &grand : child)
			{
				// Store the text of this choice. By default, the choice will
				// just bring you to the next node in the script.
				nodes.back().data.emplace_back(grand.Token(0), nodes.size());
				nodes.back().data.back().first += '\n';
				
				// If this choice contains a goto, record it.
				for(const DataFile::Node &great : grand)
				{
					int index = TokenIndex(great.Token(0));
					
					if(!index && great.Size() >= 2)
						Goto(great.Token(1), nodes.size() - 1, nodes.back().data.size() - 1);
					else if(index < 0)
						nodes.back().data.back().second = index;
					else
						continue;
					
					break;
				}
			}
			if(nodes.back().data.empty())
			{
				cerr << "Warning: conversation \"" << identifier
					<< "\" contains an empty \"choice\" node. Deleting it." << endl;
				nodes.pop_back();
			}
		}
		else if(child.Token(0) == "name")
			nodes.emplace_back(true);
		else
		{
			// This is just an ordinary text node.
			// If the previous node is a choice, or if the previous node ended
			// in a goto, create a new node. Otherwise, just merge this new
			// paragraph into the previous node.
			if(nodes.empty() || !nodes.back().canMergeOnto)
			{
				nodes.emplace_back();
				int next = nodes.size();
				nodes.back().data.emplace_back("", next);
			}
			
			nodes.back().data.back().first += child.Token(0);
			nodes.back().data.back().first += '\n';
			
			// Check if this node contains a "goto".
			for(const DataFile::Node &grand : child)
			{
				int index = TokenIndex(grand.Token(0));
					
				if(!index && grand.Size() >= 2)
					Goto(grand.Token(1), nodes.size() - 1);
				else if(index < 0)
					nodes.back().data.back().second = index;
				else
					continue;
				
				nodes.back().canMergeOnto = false;
				break;
			}
		}
	}
	
	// Display a warning if a label was not resolved.
	if(!unresolved.empty())
		for(const auto &it : unresolved)
			cerr << "Warning: in conversation \"" << identifier << "\": label \""
				<< it.first << "\" is referred to but never defined." << endl;
	
	// Check for any loops in the conversation.
	for(const auto &it : labels)
	{
		int node = it.second;
		while(node >= 0 && Choices(node) <= 1)
		{
			node = NextNode(node);
			if(node == it.second)
			{
				cerr << "Error: conversation \"" << identifier
					<< "\" contains an infinite loop beginning with label \""
					<< it.first << "\". The conversation data has been cleared." << endl;
				nodes.clear();
				return;
			}
		}
	}
	
	// Free the working buffers that we no longer need.
	labels.clear();
	unresolved.clear();
}



bool Conversation::IsChoice(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return false;
	
	return nodes[node].isChoice;
}



// The beginning of the conversation is node 0. Some nodes have choices for
// the user to select; others just automatically continue to another node.
int Conversation::Choices(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return 0;
	
	return nodes[node].isChoice ? nodes[node].data.size() : 0;
}



const string &Conversation::Text(int node, int choice) const
{
	static const string empty;
	
	if(static_cast<unsigned>(node) >= nodes.size()
			|| static_cast<unsigned>(choice) >= nodes[node].data.size())
		return empty;
	
	return nodes[node].data[choice].first;
}



int Conversation::NextNode(int node, int choice) const
{
	if(static_cast<unsigned>(node) >= nodes.size()
			|| static_cast<unsigned>(choice) >= nodes[node].data.size())
		return -2;
	
	return nodes[node].data[choice].second;
}



const Sprite *Conversation::Scene() const
{
	return scene;
}



// You can merge further text nodes onto a node only if it is not a choice and
// does not specify a goto.
Conversation::Node::Node(bool isChoice)
	: isChoice(isChoice), canMergeOnto(!isChoice)
{
}




// Add a label, pointing to whatever node is created next.
void Conversation::AddLabel(const string &label)
{
	if(labels.find(label) != labels.end())
	{
		cerr << "Warning: in conversation \"" << identifier << "\": label \""
			<< label << "\" is used more than once." << endl;
		return;
	}
	
	// If there are any unresolved references to this label, we can now set
	// their indices correctly.
	auto range = unresolved.equal_range(label);
	
	for(auto it = range.first; it != range.second; ++it)
		nodes[it->second.first].data[it->second.second].second = nodes.size();
	
	unresolved.erase(range.first, range.second);
	
	// Remember what index this label points to.
	labels[label] = nodes.size();
}



// Set up a "goto". Depending on whether the named label has been seen yet
// or not, it is either resolved immediately or added to the unresolved set.
void Conversation::Goto(const string &label, int node, int choice)
{
	auto it = labels.find(label);
	
	if(it == labels.end())
		unresolved.insert({label, {node, choice}});
	else
		nodes[node].data[choice].second = it->second;
}



// Get the index of the given special string. 0 means it is "goto", a number
// less than 0 means it is an outcome, and 1 means no match.
int Conversation::TokenIndex(const string &token)
{
	if(token == "accept")
		return ACCEPT;
	if(token == "decline")
		return DECLINE;
	if(token == "die")
		return DIE;
	
	return 0;
}
