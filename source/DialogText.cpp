/* DialogText.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DialogText.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Format.h"

using namespace std;



void DialogText::Load(const DataNode &node)
{
	//Make sure this is really a Dialog node.
	if(node.Token(0) != "dialog")
		return;
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "random")
		{
			if(child.HasChildren())
			{
				nodes.emplace_back();
				nodes.back().isRandom = true;
				int currentSet = randomSets.size();
				nodes.back().randomSet = currentSet;
				randomSets.emplace_back();
				for(const DataNode &grand : child)
				{
					if(nodes.size() > 1 && child.Size() >= 2 && child.Token(1) == "inline")
						randomSets[currentSet].emplace_back(grand.Token(0),1);
					else
						randomSets[currentSet].emplace_back("\n\t" + grand.Token(0),1);
				}
			}
		}
		else {
			std::string dialogText;
			for(int i = 1;i<child.Size();i++)
			{
				if(!dialogText.empty() || (!nodes.empty() && child.Token(0) != "inline"))
					dialogText += "\n\t";
				dialogText += child.Token(i);
			}
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
				{
					if(!dialogText.empty() || (!nodes.empty() && child.Token(0) != "inline"))
						dialogText += "\n\t";
					dialogText += grand.Token(i);
				}
			if(!dialogText.empty())
			{
				nodes.emplace_back();
				nodes.back().text = dialogText;
				if(child.Token(0) == "inline")
					nodes.back().isInline = true;
			}
		}
	}
}



void DialogText::Save(DataWriter &out) const
{
	
}



bool DialogText::IsEmpty() const
{
	return nodes.empty();
}



DialogText DialogText::Instantiate(const map<string, string> &subs) const
{
	DialogText result = *this;
	for(Node &node : result.nodes)
	{
		if(node.isRandom)
		{
			
		}
		node.text = Format::Replace(node.text,subs);
	}
	result.randomSets.clear();
	return result;
}



std::string DialogText::Text() const
{
	std::string result;
	for(Node node : nodes)
		result += node.text;
	return result;
}
