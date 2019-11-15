/* Phrase.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PHRASE_H_
#define PHRASE_H_

#include <functional>
#include <string>
#include <utility>
#include <vector>

class DataNode;



// Class representing a set of rules for generating text strings from words.
class Phrase {
public:
	void Load(const DataNode &node);
	
	const std::string &Name() const;
	std::string Get() const;
	
	
private:
	bool ReferencesPhrase(const Phrase *phrase) const;
	
	
private:
	class Part {
	public:
		std::vector<std::vector<std::pair<std::string, const Phrase*>>> words;
		std::vector<std::function<std::string(const std::string&)>> replaceRules;
	};
	
	
private:
	std::string name;
	std::vector<std::vector<Part>> parts;
};



#endif
