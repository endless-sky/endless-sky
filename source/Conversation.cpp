/* Conversation.cpp
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

#include "Conversation.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"
#include "Phrase.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"

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



// Construct and Load() at the same time.
Conversation::Conversation(const DataNode &node, const ConditionsStore *playerConditions)
{
	Load(node, playerConditions);
}



// Load a conversation from file.
void Conversation::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	// Make sure this really is a conversation specification.
	if(node.Token(0) != "conversation")
		return;

	// Free any previously loaded data.
	nodes.clear();

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "scene" && hasValue)
		{
			// A scene always starts a new text node.
			AddNode();
			nodes.back().scene = SpriteSet::Get(child.Token(1));
		}
		else if(key == "label" && hasValue)
		{
			// You cannot merge text above a label with text below it.
			if(!nodes.empty())
				nodes.back().canMergeOnto = false;
			AddLabel(child.Token(1), child);
		}
		else if(key == "choice")
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
				nodes.back().elements.emplace_back(grand.Token(0) + '\n', nodes.size());

				LoadDestinations(grand, playerConditions);
			}
			if(nodes.back().elements.empty())
			{
				if(!foundErrors)
					child.PrintTrace("Conversation contains an empty \"choice\" node:");
				nodes.pop_back();
			}
		}
		else if(key == "name")
		{
			// A name entry field is just represented as an empty choice node.
			nodes.emplace_back(true);
		}
		else if(key == "branch")
		{
			// Don't merge "branch" nodes with any other nodes.
			nodes.emplace_back();
			nodes.back().canMergeOnto = false;
			// Maintenance note: empty conditions might have to be removed in the future,
			// so their support is unofficial.
			if(child.HasChildren())
				nodes.back().conditions.Load(child, playerConditions);
			// A branch should always specify what node to go to if the test is
			// true, and may also specify where to go if it is false.
			for(int i = 1; i <= 2; ++i)
			{
				// If no link is provided, just go to the next node.
				nodes.back().elements.emplace_back("", nodes.size());
				if(child.Size() > i)
				{
					int index = TokenIndex(child.Token(i));
					if(!index)
						Goto(child.Token(i), nodes.size() - 1, i - 1);
					else if(index < 0)
						nodes.back().elements.back().next = index;
				}
			}
		}
		else if(key == "goto" && hasValue)
		{
			// Goto the label with the specified name, even if that name matches an endpoint.
			nodes.emplace_back();
			nodes.back().canMergeOnto = false;
			nodes.back().elements.emplace_back("", nodes.size());
			Goto(child.Token(1), nodes.size() - 1, 0);
		}
		else if(key == "action" || key == "apply")
		{
			if(key == "apply")
				child.PrintTrace("`apply` is deprecated syntax. Use `action` instead to ensure future compatibility.");
			// Don't merge "action" nodes with any other nodes. Allow the legacy keyword "apply," too.
			AddNode();
			nodes.back().canMergeOnto = false;
			nodes.back().actions.Load(child, playerConditions);
		}
		else if(hasValue)
			child.PrintTrace("Conversation text should be a single token:");
		else
		{
			// This is just an ordinary text node.
			// If the previous node is a choice, or if the previous node ended
			// in a goto, or if the new node has a condition, then create a new
			// node. Otherwise, just merge this new paragraph into the previous
			// node.
			if(nodes.empty() || !nodes.back().canMergeOnto || HasDisplayRestriction(child))
				AddNode();

			// Always append a newline to the end of the text.
			nodes.back().elements.back().text += key + '\n';

			// Check whether there is a goto attached to this block of text. If
			// so, future nodes can't merge onto this one.
			if(LoadDestinations(child, playerConditions))
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
			nodeIndex = NextNodeForChoice(nodeIndex);
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
			if(IsBranch(i))
			{
				out.Write("branch", TokenName(node.elements[0].next), TokenName(node.elements[1].next));
				// Write the condition set as a child of this node.
				out.BeginChild();
				{
					node.conditions.Save(out);
				}
				out.EndChild();
				continue;
			}
			if(!node.actions.IsEmpty())
			{
				out.Write("action");
				// Write the GameAction as a child of this node.
				out.BeginChild();
				{
					node.actions.Save(out);
				}
				out.EndChild();
				continue;
			}
			if(node.isChoice)
			{
				out.Write(node.elements.empty() ? "name" : "choice");
				out.BeginChild();
			}
			for(const auto &it : node.elements)
			{
				// Break the text up into paragraphs.
				for(const string &line : Format::Split(it.text, "\n"))
				{
					out.Write(line);
					// If the conditions are the same, output them for each
					// paragraph. (We currently don't merge paragraphs with
					// identical ConditionSets, but some day we might.)
					if(!it.toDisplay.IsEmpty())
					{
						out.BeginChild();
						{
							out.Write("to", "display");
							out.BeginChild();
							{
								it.toDisplay.Save(out);
							}
							out.EndChild();
						}
						out.EndChild();
					}
					if(!it.toActivate.IsEmpty())
					{
						out.BeginChild();
						{
							out.Write("to", "activate");
							out.BeginChild();
							{
								it.toActivate.Save(out);
							}
							out.EndChild();
						}
						out.EndChild();
					}
				}
				// Check what node the conversation goes to after this.
				int index = it.next;
				if(index > 0 && !NodeIsValid(index))
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
		return node.isChoice && node.elements.empty();
	});
}



// Check if the actions in this conversation are valid.
string Conversation::Validate() const
{
	for(const Node &node : nodes)
	{
		if(!node.actions.IsEmpty())
		{
			string reason = node.actions.Validate();
			if(!reason.empty())
				return "conversation action " + std::move(reason);
		}
	}
	return "";
}



// Do text replacement throughout this conversation, and instantiate any
// potential actions.
Conversation Conversation::Instantiate(map<string, string> &subs, int jumps, int payload) const
{
	Conversation result = *this;
	for(Node &node : result.nodes)
	{
		for(Element &element : node.elements)
			element.text = Format::Replace(Phrase::ExpandPhrases(element.text), subs);
		if(!node.actions.IsEmpty())
			node.actions = node.actions.Instantiate(subs, jumps, payload);
	}

	return result;
}



// Check if the given conversation node is a choice node.
bool Conversation::IsChoice(int node) const
{
	if(!NodeIsValid(node))
		return false;

	return nodes[node].isChoice;
}



// Check if the given conversation node is a choice node.
bool Conversation::HasAnyChoices(int node) const
{
	if(!NodeIsValid(node))
		return false;

	if(!nodes[node].isChoice)
		return false;

	if(nodes[node].elements.empty())
		// A zero-length choice is a special case: it sets the player's name.
		return true;

	for(const auto &data : nodes[node].elements)
	{
		if(data.toDisplay.IsEmpty())
			return true;
		if(data.toDisplay.Test())
			return true;
	}

	return false;
}



// If the given node is a choice node, check how many choices it offers.
int Conversation::Choices(int node) const
{
	if(!NodeIsValid(node))
		return 0;

	return nodes[node].isChoice ? nodes[node].elements.size() : 0;
}



bool Conversation::ChoiceIsActive(int node, int element) const
{
	if(!NodeIsValid(node) || !IsChoice(node) || !ElementIsValid(node, element))
		return false;

	return nodes[node].elements[element].toActivate.Test();
}



// Check if the given conversation node is a conditional branch.
bool Conversation::IsBranch(int node) const
{
	if(!NodeIsValid(node))
		return false;

	return !nodes[node].conditions.IsEmpty() && nodes[node].elements.size() > 1;
}



// Check if the given conversation node performs an action.
bool Conversation::IsAction(int node) const
{
	if(!NodeIsValid(node))
		return false;

	return !nodes[node].actions.IsEmpty();
}



// Get the list of conditions that the given node tests.
const ConditionSet &Conversation::Conditions(int node) const
{
	static ConditionSet empty;
	if(!NodeIsValid(node))
		return empty;

	return nodes[node].conditions;
}



// Get the action that the given node applies.
const GameAction &Conversation::GetAction(int node) const
{
	static GameAction empty;
	if(!NodeIsValid(node))
		return empty;

	return nodes[node].actions;
}



// Get the text of the given element of the given node.
const string &Conversation::Text(int node, int element) const
{
	static const string empty;

	if(!NodeIsValid(node) || !ElementIsValid(node, element))
		return empty;

	return nodes[node].elements[element].text;
}



// Get the scene image, if any, associated with the given node.
const Sprite *Conversation::Scene(int node) const
{
	if(!NodeIsValid(node))
		return nullptr;

	return nodes[node].scene;
}



// Find out where the conversation goes if the given option is chosen.
int Conversation::NextNodeForChoice(int node, int element) const
{
	if(!NodeIsValid(node) || !ElementIsValid(node, element))
		return DECLINE;

	return nodes[node].elements[element].next;
}



// Go to the next node of the conversation, ignoring any choices.
int Conversation::StepToNextNode(int node) const
{
	int next_node = node+1;

	if(!NodeIsValid(next_node))
		return DECLINE;

	return next_node;
}



// Returns whether the given node should be displayed.
bool Conversation::ShouldDisplayNode(int node, int element) const
{
	if(!NodeIsValid(node))
		return false;
	else if(IsChoice(node) ? !ElementIsValid(node, element) : element != 0)
		return false;
	const auto &data = nodes[node].elements[element];
	if(data.toDisplay.IsEmpty())
		return true;
	return data.toDisplay.Test();
}



// Returns true if the given node index is in the range of valid nodes for this
// Conversation.
bool Conversation::NodeIsValid(int node) const
{
	if(node < 0)
		return false;
	return static_cast<unsigned>(node) < nodes.size();
}



// Returns true if the given node index is in the range of valid nodes for this
// Conversation *and* the given element index is in the range of valid elements
// for the given node.
bool Conversation::ElementIsValid(int node, int element) const
{
	if(!NodeIsValid(node))
		return false;
	else if(element < 0)
		return false;
	return static_cast<unsigned>(element) < nodes[node].elements.size();
}



// Parse the children of the given node to see if then contain any "goto" or
// "to display" nodes. If so, link them up properly. Return true if gotos or
// conditions were found.
bool Conversation::LoadDestinations(const DataNode &node, const ConditionsStore *playerConditions)
{
	bool hasGoto = false;
	bool hasDisplayCondition = false;
	bool hasActivationCondition = false;
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "goto" && hasValue)
		{
			if(hasGoto)
				child.PrintTrace("Ignoring extra endpoint in conversation choice:");
			else
			{
				Goto(child.Token(1), nodes.size() - 1, nodes.back().elements.size() - 1);
				hasGoto = true;
			}
		}
		else if(key == "to" && hasValue && child.Token(1) == "display")
		{
			if(hasDisplayCondition)
				child.PrintTrace("Ignoring extra condition in conversation choice:");
			else
			{
				nodes.back().elements.back().toDisplay.Load(child, playerConditions);
				hasDisplayCondition = true;
			}
		}
		else if(key == "to" && hasValue && child.Token(1) == "activate")
		{
			if(hasActivationCondition)
				child.PrintTrace("Ignoring extra condition in conversation choice:");
			else
			{
				nodes.back().elements.back().toActivate.Load(child, playerConditions);
				hasActivationCondition = true;
			}
		}
		else
		{
			// Check if this is a recognized endpoint name.
			int index = TokenIndex(key);
			if(!hasValue && index < 0)
			{
				if(hasGoto)
					child.PrintTrace("Ignoring extra endpoint in conversation choice:");
				else
				{
					nodes.back().elements.back().next = index;
					hasGoto = true;
				}
			}
			else
				child.PrintTrace("Expected goto, to display, or endpoint in conversation, found this:");
		}
	}
	return hasGoto || hasDisplayCondition;
}



bool Conversation::HasDisplayRestriction(const DataNode &node)
{
	for(const DataNode &child : node)
		if(child.Token(0) == "to" && child.Size() >= 2 && child.Token(1) == "display")
			return true;

	return false;
}



// Add a label, pointing to whatever node is created next.
void Conversation::AddLabel(const string &label, const DataNode &node)
{
	if(labels.contains(label))
	{
		node.PrintTrace("Conversation: label \"" + label + "\" is used more than once:");
		return;
	}

	// If there are any unresolved references to this label, we can now set
	// their indices correctly.
	auto range = unresolved.equal_range(label);

	for(auto it = range.first; it != range.second; ++it)
		nodes[it->second.first].elements[it->second.second].next = nodes.size();

	unresolved.erase(range.first, range.second);

	// Remember what index this label points to.
	labels[label] = nodes.size();
}



// Set up a "goto". Depending on whether the named label has been seen yet
// or not, it is either resolved immediately or added to the unresolved set.
void Conversation::Goto(const string &label, int node, int element)
{
	auto it = labels.find(label);

	if(it == labels.end())
		unresolved.insert({label, {node, element}});
	else
		nodes[node].elements[element].next = it->second;
}



// Add an "empty" node. It will contain one empty line of text, with its
// goto link set to fall through to the next node.
void Conversation::AddNode()
{
	nodes.emplace_back();
	nodes.back().elements.emplace_back("", nodes.size());
}
