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
#include "Random.h"

using namespace std;



void DialogText::Load(const DataNode &node)
{
	//Make sure this is really a Dialog node.
	if(node.Token(0) != "dialog")
		return;
	
	string dialogText;
	for(int i = 1;i<node.Size();i++)
	{
		if(!dialogText.empty() || !nodes.empty())
			dialogText += "\n\t";
		dialogText += node.Token(i);
	}
	if(!dialogText.empty())
	{
		nodes.emplace_back();
		nodes.back().text = dialogText;
	}
	
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
					int probability = 1;
					if(grand.Size() >= 2)
						probability = grand.Value(1);
					if(nodes.size() > 1 && child.Size() >= 2 && child.Token(1) == "inline")
						randomSets[currentSet].emplace_back(pair<string,int>(grand.Token(0),probability));
					else
						randomSets[currentSet].emplace_back(pair<string,int>("\n\t" + grand.Token(0),probability));
					nodes.back().probability += probability;
				}
			}
		}
		else if(child.Token(0) == "inline")
		{
			string text;
			for(int i = 1;i<child.Size();i++)
			{
				if(!text.empty())
					text += "\n\t";
				text += child.Token(i);
			}
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
				{
					if(!text.empty())
						text += "\n\t";
					text += grand.Token(i);
				}
			if(!text.empty())
			{
				nodes.emplace_back();
				nodes.back().text = text;
			}
		}
		else {
			string text;
			for(int i = 0;i<child.Size();i++)
			{
				if(!text.empty() || !nodes.empty())
					text += "\n\t";
				text += child.Token(i);
			}
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
				{
					if(!text.empty() || !nodes.empty())
						text += "\n\t";
					text += grand.Token(i);
				}
			if(!text.empty())
			{
				nodes.emplace_back();
				nodes.back().text = text;
			}
		}
	}
}



void DialogText::Save(DataWriter &out) const
{
	out.Write("dialog");
	out.BeginChild();
	{
		// Break the text up into paragraphs.
		size_t begin = 0;
		string dialogText = Text();
		while(true)	
		{
			size_t pos = dialogText.find("\n\t", begin);
			if(pos == string::npos)
				pos = dialogText.length();
			out.Write(dialogText.substr(begin, pos - begin));
			if(pos == dialogText.length())
				break;
			begin = pos + 2;
		}
	}
	out.EndChild();
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
			int probabilityChoice = Random::Int(node.probability);
			int theset = node.randomSet;
			vector<pair<string,int>> randomSet = result.randomSets[node.randomSet];
			for(pair<string,int> &option : randomSet)
			{
				if(probabilityChoice < option.second)
				{
					node.text = Format::Replace(option.first,subs);
					node.isRandom = false;
					break;
				}
				else
					probabilityChoice -= option.second;
			}
		}
		node.text = Format::Replace(node.text,subs);
	}
	result.randomSets.clear();
	return result;
}



string DialogText::Text() const
{
	string result;
	for(Node node : nodes)
		result += node.text;
	return result;
}
