/* Conversation.h
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

#pragma once

#include "ConditionSet.h"
#include "GameAction.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class ConditionsStore;
class DataNode;
class DataWriter;
class Sprite;



// Class representing a conversation, generally occurring when the player is asked to
// accept or decline a mission. The conversation can take different paths depending
// on what responses you choose, leading you to accept, decline, or (rarely) to be
// killed. A conversation can also branch based on various condition flags that
// are set for the player, or even trigger various changes to the game's state.
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
	Conversation() = default;
	// Construct and Load() at the same time.
	Conversation(const DataNode &node, const ConditionsStore *playerConditions);
	// Read or write to files.
	void Load(const DataNode &node, const ConditionsStore *playerConditions);
	void Save(DataWriter &out) const;
	// Check if any data is loaded in this conversation object.
	bool IsEmpty() const noexcept;
	// Check if this conversation includes a name prompt.
	bool IsValidIntro() const noexcept;
	// Check if the actions in this conversation are valid.
	std::string Validate() const;

	// Generate a new conversation from this template, filling in any text
	// substitutions and instantiating any actions.
	Conversation Instantiate(std::map<std::string, std::string> &subs, int jumps = 0, int payload = 0) const;

	// The beginning of the conversation is node 0. Some nodes have choices for
	// the user to select; others just automatically continue to another node.
	// Nodes may also display images or include conditional branches.
	bool IsChoice(int node) const;
	// Some choices have conditions in each option. If all options are disabled,
	// the choice cannot be shown.
	bool HasAnyChoices(int node) const;
	// If the given node is a choice node, check how many choices it offers.
	int Choices(int node) const;
	// Determine if the given choice is active. Inactive choices cannot be selected.
	bool ChoiceIsActive(int node, int element) const;
	// Check if the given conversation node is a conditional branch.
	bool IsBranch(int node) const;
	// Check if the given conversation node performs an action.
	bool IsAction(int node) const;
	const ConditionSet &Conditions(int node) const;
	const GameAction &GetAction(int node) const;
	const std::string &Text(int node, int element = 0) const;
	const Sprite *Scene(int node) const;
	// Find out where the conversation goes if the given option is chosen.
	int NextNodeForChoice(int node, int element = 0) const;
	// Go to the next node of the conversation, ignoring any choices.
	int StepToNextNode(int node) const;
	// Returns whether the given node should be displayed.
	// Returns false if:
	// - The node (or element) is out of range.
	// - The node is not a choice node, and element is non-zero.
	// - The node (or element) has conditions and those conditions are not met.
	// and true otherwise.
	bool ShouldDisplayNode(int node, int element = 0) const;
	// Returns true if the given node index is in the range of valid nodes for
	// this Conversation.
	// Note: This function only considers actual Conversation nodes to be valid
	// choices. The negative "outcome" values (ACCEPT, DEFER, etc.) are special
	// sentinel values, rather than indices of Conversation nodes, and are
	// therefore considered invalid by this function.
	bool NodeIsValid(int node) const;
	// Returns true if the given node index is in the range of valid nodes for
	// this Conversation *and* the given element index is in the range of valid
	// elements for the given node.
	bool ElementIsValid(int node, int element) const;


private:
	// This serves multiple purposes:
	// - In a regular text node, there's exactly one of these. It contains the
	//   text data, the index of the next node to unconditionally visit, and,
	//   optionally, a condition set which, if not met, prevents the text from
	//   being displayed (without affecting which node is processed next).
	// - In a choice node, there's one of these for each possible choice,
	//   containing the text to display, the node the choice leads to, and,
	//   optionally, the conditions under which to offer the choice.
	// - In a branch node, there's two of these. The first one contains the
	//   condition for the branch. If the condition is met, the "next" member
	//   of the first element is followed. If it's not met, it's the second
	//   element whose "next" member is followed.
	class Element {
	public:
		explicit Element(std::string text, int next)
			: text(std::move(text)), next(next) {}
		// The text to display:
		std::string text;
		// The next node to visit:
		int next;
		// Conditions for displaying or activating text:
		ConditionSet toDisplay;
		ConditionSet toActivate;
	};

	// The conversation is a network of "nodes" that you travel between by
	// making choices (or by automatic branches that depend on the condition
	// variable values for the current player).
	class Node {
	public:
		// Construct a new node. Each paragraph of conversation that involves no
		// choice can be merged into what came before it, to simplify things.
		explicit Node(bool isChoice = false) noexcept : isChoice(isChoice), canMergeOnto(!isChoice) {}

		// The condition expressions that determine the next node to load, or
		// whether to display.
		ConditionSet conditions;
		// Tasks performed when this node is reached.
		GameAction actions;
		// See Element's comment above for what this actually entails.
		std::vector<Element> elements;
		// This distinguishes "choice" nodes from "branch" or text nodes. If
		// this value is false, a one-element node is considered text, and a
		// node with more than one element is considered a "branch".
		bool isChoice;
		// Keep track of whether it's possible to merge future nodes onto this.
		bool canMergeOnto;

		// Image that should be shown along with this text.
		const Sprite *scene = nullptr;
	};


private:
	// Parse the children of the given node to see if they contain any "gotos"
	// or "conditions." If so, link them up properly. Return true if gotos or
	// conditions were found.
	bool LoadDestinations(const DataNode &node, const ConditionsStore *playerConditions);
	// Parse the children to see if there is a condition.
	bool HasDisplayRestriction(const DataNode &node);
	// Add a label, pointing to whatever node is created next.
	void AddLabel(const std::string &label, const DataNode &node);
	// Set up a "goto". Depending on whether the named label has been seen yet
	// or not, it is either resolved immediately or added to the unresolved set.
	void Goto(const std::string &label, int node, int element = 0);
	// Add an "empty" node. It will contain one empty line of text, with its
	// goto link set to fall through to the next node.
	void AddNode();


private:
	// While parsing the conversation, keep track of what labels link to what
	// nodes. If a name appears in a goto before that label appears, remember
	// what node and what element it appeared at in order to link it up later.
	std::map<std::string, int> labels;
	std::multimap<std::string, std::pair<int, int>> unresolved;
	// The actual conversation data:
	std::vector<Node> nodes;
};
