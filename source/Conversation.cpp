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
#include "Format.h"
#include "SpriteSet.h"

using namespace std;

namespace {
	// Get the index of the given special string. 0 means it is "goto", a number
	// less than 0 means it is an outcome, and 1 means no match.
	static int TokenIndex(const string &token)
	{
		if(token == "accept")
			return Conversation::ACCEPT;
		if(token == "launch")
			return Conversation::LAUNCH;
		if(token == "decline")
			return Conversation::DECLINE;
		if(token == "flee")
			return Conversation::FLEE;
		if(token == "defer")
			return Conversation::DEFER;
		if(token == "depart")
			return Conversation::DEPART;
		if(token == "die")
			return Conversation::DIE;
		
		return 0;
	}

	static string TokenName(int index)
	{
		if(index == Conversation::ACCEPT)
			return "accept";
		else if(index == Conversation::LAUNCH)
			return "launch";
		else if(index == Conversation::DECLINE)
			return "decline";
		else if(index == Conversation::FLEE)
			return "flee";
		else if(index == Conversation::DEFER)
			return "defer";
		else if(index == Conversation::DEPART)
			return "depart";
		else if(index == Conversation::DIE)
			return "die";
		else
			return to_string(index);
	}
	
	static void WriteToken(int index, DataWriter &out)
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



// Public utility function
bool Conversation::LeaveImmediately(int outcome)
{
	return outcome == LAUNCH || outcome == FLEE || outcome == DEPART;
}



void Conversation::Load(const DataNode &node)
{
	if(node.Token(0) != "conversation")
		return;
	if(node.Size() >= 2)
		identifier = node.Token(1);
	
	// Free any previously loaded data.
	nodes.clear();
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "scene" && child.Size() >= 2)
		{
			nodes.emplace_back();
			int next = nodes.size();
			nodes.back().data.emplace_back("", next);
			
			nodes.back().scene = SpriteSet::Get(child.Token(1));
			nodes.back().sceneName = child.Token(1);
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
			for(const DataNode &grand : child)
			{
				// Store the text of this choice. By default, the choice will
				// just bring you to the next node in the script.
				nodes.back().data.emplace_back(grand.Token(0), nodes.size());
				nodes.back().data.back().first += '\n';
				
				// If this choice contains a goto, record it.
				for(const DataNode &great : grand)
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
				child.PrintTrace("Conversation contains an empty \"choice\" node:");
				nodes.pop_back();
			}
		}
		else if(child.Token(0) == "name")
			nodes.emplace_back(true);
		else if(child.Token(0) == "branch")
		{
			nodes.emplace_back();
			nodes.back().canMergeOnto = false;
			nodes.back().conditions.Load(child);
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
			nodes.emplace_back();
			nodes.back().canMergeOnto = false;
			nodes.back().conditions.Load(child);
			nodes.back().data.emplace_back("", nodes.size());
			if(child.Size() > 1)
			{
				int index = TokenIndex(child.Token(1));
				if(!index)
					Goto(child.Token(1), nodes.size() - 1, 0);
				else if(index < 0)
					nodes.back().data.back().second = index;
			}
		}
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
			for(const DataNode &grand : child)
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
			node.PrintTrace("Conversation contains unused label \"" + it.first + "\":");
	
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



void Conversation::Save(DataWriter &out) const
{
	if(!identifier.empty())
		out.Write("conversation", identifier);
	else
		out.Write("conversation");
	out.BeginChild();
	{
		for(unsigned i = 0; i < nodes.size(); ++i)
		{
			out.Write("label", i);
			const Node &node = nodes[i];
			
			if(node.scene)
				out.Write("scene", node.sceneName);	
			if(!node.conditions.IsEmpty())
			{
				if(node.data.size() > 1)
					out.Write("branch", TokenName(node.data[0].second), TokenName(node.data[1].second));
				else
					out.Write("apply", TokenName(node.data[0].second));
				
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
				size_t begin = 0;
				while(begin != it.first.length())
				{
					size_t pos = it.first.find('\n', begin);
					if(pos == string::npos)
						pos = it.first.length();
					out.Write(it.first.substr(begin, pos - begin));
					if(pos == it.first.length())
						break;
					begin = pos + 1;
				}
				int index = it.second;
				if(index > 0 && static_cast<unsigned>(index) >= nodes.size())
					index = -1;
				
				WriteToken(index, out);
			}
			if(node.isChoice)
				out.EndChild();
		}
	}
	out.EndChild();
}



bool Conversation::IsEmpty() const
{
	return nodes.empty();
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



bool Conversation::IsBranch(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return false;
	
	return !nodes[node].conditions.IsEmpty() && nodes[node].data.size() > 1;
}




bool Conversation::IsApply(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return false;
	
	return !nodes[node].conditions.IsEmpty() && nodes[node].data.size() == 1;
}



const ConditionSet &Conversation::Conditions(int node) const
{
	static ConditionSet empty;
	if(static_cast<unsigned>(node) >= nodes.size())
		return empty;
	
	return nodes[node].conditions;
}



const string &Conversation::Text(int node, int choice) const
{
	static const string empty;
	
	if(static_cast<unsigned>(node) >= nodes.size()
			|| static_cast<unsigned>(choice) >= nodes[node].data.size())
		return empty;
	
	return nodes[node].data[choice].first;
}



const Sprite *Conversation::Scene(int node) const
{
	if(static_cast<unsigned>(node) >= nodes.size())
		return nullptr;
	
	return nodes[node].scene;
}



int Conversation::NextNode(int node, int choice) const
{
	if(static_cast<unsigned>(node) >= nodes.size()
			|| static_cast<unsigned>(choice) >= nodes[node].data.size())
		return -2;
	
	return nodes[node].data[choice].second;
}



// You can merge further text nodes onto a node only if it is not a choice and
// does not specify a goto.
Conversation::Node::Node(bool isChoice)
	: isChoice(isChoice), canMergeOnto(!isChoice)
{
}




// Add a label, pointing to whatever node is created next.
void Conversation::AddLabel(const string &label, const DataNode &node)
{
	if(labels.find(label) != labels.end())
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
