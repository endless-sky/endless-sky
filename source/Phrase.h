/* Phrase.h
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

#include "WeightedList.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

class DataNode;



// Class representing a set of rules for generating text strings from words.
class Phrase {
public:
	// Replace all occurrences ${phrase name} with the expanded phrase from GameData::Phrases()
	static std::string ExpandPhrases(const std::string &source);


public:
	Phrase() = default;
	// Construct and Load() at the same time.
	explicit Phrase(const DataNode &node);

	// Parse the given node into a new branch associated with this phrase.
	void Load(const DataNode &node);

	bool IsEmpty() const;

	const std::string &Name() const;
	std::string Get() const;


private:
	bool ReferencesPhrase(const Phrase *phrase) const;


private:
	// A Choice represents one entry in a Phrase definition's "word" or "phrase" child
	// node. If from a "word" node, a Choice may be pure text or contain embedded phrase
	// references, e.g. `"I'm ${pirate} and I like '${band}' concerts."`.
	class Choice : private std::vector<std::pair<std::string, const Phrase *>> {
	public:
		// Create a choice from a grandchild DataNode.
		explicit Choice(const DataNode &node, bool isPhraseName = false);

		// Enable empty checks and iteration:
		using std::vector<std::pair<std::string, const Phrase *>>::empty;
		using std::vector<std::pair<std::string, const Phrase *>>::begin;
		using std::vector<std::pair<std::string, const Phrase *>>::end;
	};


	// A Part represents the content contained by a "word", "phrase", or "replace" child node.
	class Part {
	public:
		// Sources of text, either literal or via phrase invocation.
		WeightedList<Choice> choices;
		// Character sequences that should be replaced, e.g. "llo"->"y"
		// would transform "Hello hello" into "Hey hey"
		std::vector<std::pair<std::string, std::string>> replacements;
	};


	// An individual definition associated with a Phrase name.
	class Sentence : private std::vector<Part> {
	public:
		Sentence(const DataNode &node, const Phrase *parent);
		void Load(const DataNode &node, const Phrase *parent);

		// Enable empty checks and iteration:
		using std::vector<Part>::empty;
		using std::vector<Part>::begin;
		using std::vector<Part>::end;
	};


private:
	std::string name;
	// Each time this phrase is defined, a new sentence is created.
	std::vector<Sentence> sentences;
};
