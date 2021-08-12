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

#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"
#include "Sprite.h"
#include "SpriteSet.h"

using namespace std;

namespace {
	// Lookup table for matching special tokens to enumeration values.
	map<string, int> TOKEN_INDEX = {
		{"accept", Conversation::ACCEPT},
		{"decline", Conversation::DECLINE},
		{"defer", Conversation::DEFER},
		{"launch", Conversation::LAUNCH},
		{"flee", Conversation::FLEE},
		{"depart", Conversation::DEPART},
		{"die", Conversation::DIE},
		{"explode", Conversation::EXPLODE}
	};
	
	// Get the index of the given special string. 0 means it is "goto", a number
	// less than 0 means it is an outcome, and 1 means no match.
	int TokenIndex(const string &token)
	{
		auto it = TOKEN_INDEX.find(token);
		return (it == TOKEN_INDEX.end() ? 0 : it->second);
	}
	
	// Map an index back to a string, for saving the conversation to a file.
	string TokenName(int index)
	{
		for(const auto &it : TOKEN_INDEX)
			if(it.second == index)
				return it.first;
		
		return to_string(index);
	}
	
	// Write a "goto" or endpoint.
	void WriteToken(int index, DataWriter &out)
	{
		out.BeginChild();
		{
			if(index >= 0)
				out.Write("goto", index);
			else
				out.Write(TokenName(index));
		}
		out.EndChild();
	}
}

// The possible outcomes of a conversation:
const int Conversation::ACCEPT;
const int Conversation::DECLINE;
const int Conversation::DEFER;
const int Conversation::LAUNCH;
const int Conversation::FLEE;
const int Conversation::DEPART;
const int Conversation::DIE;
const int Conversation::EXPLODE;



// Check if this conversation outcome requires the player to leave immediately.
bool Conversation::RequiresLaunch(int outcome)
{
	return outcome == LAUNCH || outcome == FLEE || outcome == DEPART;
}



// Load a conversation from file.
void Conversation::Load(const DataNode &node)
{
	// Make sure this really is a conversation specification.
	if(node.Token(0) != "conversation")
		return;
	
	// Free any previously loaded data.
	nodes.clear();
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "scene" && child.Size() >= 2)
		{
			// A scene always starts a new text node.
			AddNode();
			nodes.back().scene = SpriteSet::Get(child.Token(1));
		}
		else if(child.Token(0) == "label" && child.Size() >= 2)
		{
			// You cannot merge text above a label with text below it.
			if(!nodes.empty())
				nodes.back().canMergeOnto = false;
			AddLabel(child.Token(1), child);
		}
		else if(child.Token(0) == "choice")
		{
			// Create a new node with one or more choices in it.
			nodes.emplace_back(true);
			bool foundErrors = false;
			for(const DataNode &grand : child)
			{
				// Check for common errors such as indenting a goto incorrectly:
				if(grand.Size() > 1)
				{
					grand.PrintTrace("Conversation choices should be a single token:");
					foundErrors = true;
					continue;
				}
				
				// Store the text of this choice. By default, the choice will
				// just bring you to the next node in the script.
				nodes.back().data.emplace_back(grand.Token(0), nodes.size());
				nodes.back().data.back().first += '\n';
				
				LoadGotos(grand);
			}
			if(nodes.back().data.empty())
			{
				if(!foundErrors)
					child.PrintTrace("Conversation contains an empty \"choice\" node:");
				nodes.pop_back();
			}
		}
		else if(child.Token(0) == "name")
		{
			// A name entry field is just represented as an empty choice node.
			nodes.emplace_back(true);
		}
		else if(child.Token(0) == "branch")
		{
			// Don't merge "branch" nodes with any other nodes.
			nodes.emplace_back();
			nodes.back().canMergeOnto = false;
			nodes.back().conditions.Load(child);
			// A branch should always specify what node to go to if the test is
			// true, and may also specify where to go if it is false.
			for(int i = 1; i <= 2; ++i)
			{
				// If no link is provided, just go to the next node.
				nodes.back().data.emplace_back("", nodes.size());
				if(child.Size() > i)
				{
					int index = TokenIndex(child.Token(i));
					if(!index)
						Goto(child.Token(i), nodes.size() - 1, i - 1);
					else if(index < 0)
						nodes.back().data.back().second = index;
				}
			}
		}
		else if(child.Token(0) == "apply")
		{
			// Don't merge "apply" nodes with any other nodes.
			AddNode();
			nodes.back().canMergeOnto = false;
			nodes.back().conditions.Load(child);
		}
		// Check for common errors such as indenting a goto incorrectly:
		else if(child.Size() > 1)
			child.PrintTrace("Conversation text should be a single token:");
		else
		{
			// This is just an ordinary text node.
			// If the previous node is a choice, or if the previous node ended
			// in a goto, create a new node. Otherwise, just merge this new
			// paragraph into the previous node.
			if(nodes.empty() || !nodes.back().canMergeOnto)
				AddNode();
			
			// Always append a newline to the end of the text.
			nodes.back().data.back().first += child.Token(0);
			nodes.back().data.back().first += '\n';
			
			// Check whether there is a goto attached to this block of text. If
			// so, future nodes can't merge onto this one.
			if(LoadGotos(child))
				nodes.back().canMergeOnto = false;
		}
	}
	
	// Display a warning if a label was not resolved.
	if(!unresolved.empty())
		for(const auto &it : unresolved)
			node.PrintTrace("Conversation contains unrecognized label \"" + it.first + "\":");
	
	// Check for any loops in the conversation.
	for(const auto &it : labels)
	{
		int nodeIndex = it.second;
		while(nodeIndex >= 0 && Choices(nodeIndex) <= 1)
		{
			nodeIndex = NextNode(nodeIndex);
			if(nodeIndex == it.second)
			{
				node.PrintTrace("Conversation contains infinite loop beginning with label \"" + it.first + "\":");
				nodes.clear();
				return;
			}
		}
	}
	
	// Free the working buffers that we no longer need.
	labels.clear();
	unresolved.clear();
}



// Write a conversation to file.
void Conversation::Save(DataWriter &out) const
{
	out.Write("conversation");
	out.BeginChild();
	{
		for(unsigned i = 0; i < nodes.size(); ++i)
		{
			// The original label names are not preserved anywhere. Instead,
			// the label for every node is just its node index.
			out.Write("label", i);
			const Node &node = nodes[i];
			
			if(node.scene)
				out.Write("scene", node.scene->Name());	
			if(!node.conditions.IsEmpty())
			{
				// The only thing differentiating a "branch" from an "apply" node
				// is that a branch has two data entries instead of one.
				if(node.data.size() > 1)
					out.Write("branch", TokenName(node.data[0].second), TokenName(node.data[1].second));
				else
					out.Write("apply", TokenName(node.data[0].second));
				
				// Write the condition set as a child of this node.
				out.BeginChild();
				{
					node.conditions.Save(out);
				}
				out.EndChild();
				continue;
			}
			if(node.isChoice)
			{
				out.Write(node.data.empty() ? "name" : "choice");
				out.BeginChild();
			}
			for(const auto &it : node.data)
			{
				// Break the text up into paragraphs.
				for(const string &line : Format::Split(it.first, "\n"))
					out.Write(line);
				// Check what node the conversation goes to after this.
				int index = it.second;
				if(index > 0 && static_cast<unsigned>(index) >= nodes.size())
					index = Conversation::DECLINE;
				
				// Write the node that we go to next after this.
				WriteToken(index, out);
			}
			if(node.isChoice)
				out.EndChild();
		}
	}
	out.EndChild();
}



// Check if this conversation contains any data.
bool Conversation::IsEmpty() const noexcept
{
	return nodes.empty();
}



// Check if this conversation contains a name prompt, and thus can be used as an "intro" conversation.
bool Conversation::IsValidIntro() const noexcept
{
	return any_of(nodes.begin(), nodes.end(), [](const Node &node) noexcept -> bool {
		return node.isChoice && node.data.empty();
	});
}



// Do text replacement throughout this conversation.
Conversation Conversation::Substitute(const map<string, string> &subs) const
{
	Conversation result = *this;
	for(Node &node : result.nodes)
		for(pair<string, int> &choice : node.data)
			choice.first = Format::Replace(choice.first, subs);
	return result;
}



// Check if the given conversation node is a choice node.
bool Conversation::IsChoice(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return false;
	
	return nodes[node].isChoice;
}



// If the given node is a choice node, check how many choices it offers.
int Conversation::Choices(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return 0;
	
	return nodes[node].isChoice ? nodes[node].data.size() : 0;
}



// Check if the given converation node is a conditional branch.
bool Conversation::IsBranch(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return false;
	
	return !nodes[node].conditions.IsEmpty() && nodes[node].data.size() > 1;
}




// Check if the given converation node applies changes to condition variables.
bool Conversation::IsApply(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return false;
	
	return !nodes[node].conditions.IsEmpty() && nodes[node].data.size() == 1;
}



// Get the list of conditions that the given node tests or applies.
const ConditionSet &Conversation::Conditions(int node) const
{
	static ConditionSet empty;
	if(static_cast<unsigned>(node) >= nodes.size())
		return empty;
	
	return nodes[node].conditions;
}



// Get the text of the given choice of the given node.
const string &Conversation::Text(int node, int choice) const
{
	static const string empty;
	
	if(static_cast<unsigned>(node) >= nodes.size()
			|| static_cast<unsigned>(choice) >= nodes[node].data.size())
		return empty;
	
	return nodes[node].data[choice].first;
}



// Get the scene image, if any, associated with the given node.
const Sprite *Conversation::Scene(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return nullptr;
	
	return nodes[node].scene;
}



// Find out where the conversation goes if the given option is chosen.
int Conversation::NextNode(int node, int choice) const
{
	if(static_cast<unsigned>(node) >= nodes.size()
			|| static_cast<unsigned>(choice) >= nodes[node].data.size())
		return DECLINE;
	
	return nodes[node].data[choice].second;
}



// Parse the children of the given node to see if then contain any "gotos."
// If so, link them up properly. Return true if gotos were found.
bool Conversation::LoadGotos(const DataNode &node)
{
	bool hasGoto = false;
	for(const DataNode &child : node)
	{
		if(hasGoto)
			child.PrintTrace("Ignoring extra text in conversation choice:");
		else if(child.Size() == 2 && child.Token(0) == "goto")
		{
			// Each choice can only have one goto
			Goto(child.Token(1), nodes.size() - 1, nodes.back().data.size() - 1);
			hasGoto = true;
		}
		else
		{
			// Check if this is a recognized endpoint name.
			int index = TokenIndex(child.Token(0));
			if(child.Size() == 1 && index < 0)
			{
				nodes.back().data.back().second = index;
				hasGoto = true;
			}
			else
				child.PrintTrace("Expected goto or endpoint in conversation, found this:");
		}
	}
	return hasGoto;
}



// Add a label, pointing to whatever node is created next.
void Conversation::AddLabel(const string &label, const DataNode &node)
{
	if(labels.count(label))
	{
		node.PrintTrace("Conversation: label \"" + label + "\" is used more than once:");
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



// Add an "empty" node. It will contain one empty line of text, with its
// goto link set to fall through to the next node.
void Conversation::AddNode()
{
	nodes.emplace_back();
	nodes.back().data.emplace_back("", nodes.size());
}
