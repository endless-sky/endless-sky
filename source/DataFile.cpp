/* DataFile.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DataFile.h"

#include <fstream>

using namespace std;



DataFile::DataFile(const string &path)
{
	Load(path);
}



DataFile::DataFile(istream &in)
{
	Load(in);
}



void DataFile::Load(const string &path)
{
	// Check if the file exists before doing anything with it.
	ifstream in(path);
	if(!in.is_open())
		return;
	
	// Find out how big the file is.
	in.seekg(0, ios_base::end);
	size_t size = in.tellg();
	in.seekg(0, ios_base::beg);
	
	// Allocate one extra character for the sentinel '\n' character.
	vector<char> data(size + 1);
	in.read(&*data.begin(), size);
	// As a sentinel, make sure the file always ends in a newline.
	data.back() = '\n';
	
	Load(&*data.begin(), &*data.end());
}



void DataFile::Load(istream &in)
{
	vector<char> data;
	
	static const size_t BLOCK = 4096;
	while(in)
	{
		size_t currentSize = data.size();
		data.resize(currentSize + BLOCK);
		in.read(&*data.begin() + currentSize, BLOCK);
		data.resize(currentSize + in.gcount());
	}
	// As a sentinel, make sure the file always ends in a newline.
	if(data.back() != '\n')
		data.push_back('\n');
	
	Load(&*data.begin(), &*data.end());
}



list<DataNode>::const_iterator DataFile::begin() const
{
	return root.begin();
}



list<DataNode>::const_iterator DataFile::end() const
{
	return root.end();
}



void DataFile::Load(const char *it, const char *end)
{
	vector<DataNode *> stack(1, &root);
	vector<int> whiteStack(1, -1);
	
	for( ; it != end; ++it)
	{
		// Find the first non-white character in this line.
		int white = 0;
		for( ; *it <= ' ' && *it != '\n'; ++it)
			++white;
		
		// If the line is a comment, skip to the end of the line.
		if(*it == '#')
		{
			while(*it != '\n')
				++it;
		}
		// Skip empty lines (including comment lines).
		if(*it == '\n')
			continue;
		
		// Determine where in the node tree we are inserting this node, based on
		// whether it has more indentation that the previous node, less, or the same.
		while(whiteStack.back() >= white)
		{
			whiteStack.pop_back();
			stack.pop_back();
		}
		
		// Add this node as a child of the proper node.
		list<DataNode> &children = stack.back()->children;
		children.push_back(DataNode());
		DataNode &node = children.back();
		
		// Remember where in the tree we are.
		stack.push_back(&node);
		whiteStack.push_back(white);
		
		// Tokenize the line. Skip comments and empty lines.
		while(*it != '\n')
		{
			char endQuote = *it;
			bool isQuoted = (endQuote == '"' || endQuote == '`');
			it += isQuoted;
			
			const char *start = it;
			
			// Find the end of this token.
			while(*it != '\n' && (isQuoted ? (*it != endQuote) : (*it > ' ')))
				++it;
			node.tokens.emplace_back(start, it);
			
			if(*it != '\n')
			{
				it += isQuoted;
				while(*it != '\n' && *it <= ' ' && *it != '#')
					++it;
				
				// If a comment is encountered outside of a token, skip the rest
				// of this line of the file.
				if(*it == '#')
				{
					while(*it != '\n')
						++it;
				}
			}
		}
	}
}
