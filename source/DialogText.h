/* DialogText.h,
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DIALOG_TEXT_H
#define DIALOG_TEXT_H

#include <map>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class DataWriter;



class DialogText{
public:
	// Read or write to files.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;
	// Check if any data is loaded in this dialog text object.
	bool IsEmpty() const;
	
	DialogText Instantiate(const std::map<std::string, std::string> &subs) const;
	std::string Text() const;
	
private:
	class Node {
	public:
		Node(bool isInline = false, bool isRandom = false,int randomSet = 0){};
		std::string text;
		bool isRandom;
		bool isInline;
		int randomSet;
	};
	
private:
	std::vector<std::vector<std::pair<int,int>>> randomSets;
	std::vector<Node> nodes;
};

#endif
