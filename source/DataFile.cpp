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

#include "Files.h"

using namespace std;



// Constructor, taking a file path (in UTF-8).
DataFile::DataFile(const string &path)
{
	Load(path);
}



// Constructor, taking an istream. This can be cin or a file.
DataFile::DataFile(istream &in)
{
	Load(in);
}



// Load from a file path (in UTF-8).
void DataFile::Load(const string &path)
{
	string data = Files::Read(path);
	if(data.empty())
		return;
	
	// As a sentinel, make sure the file always ends in a newline.
	if(data.empty() || data.back() != '\n')
		data.push_back('\n');
	
	// Note what file this node is in, so it will show up in error traces.
	root.tokens.push_back("file");
	root.tokens.push_back(path);
	
	Load(&*data.begin(), &*data.end());
}



// Constructor, taking an istream. This can be cin or a file.
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



// Get an iterator to the start of the list of nodes in this file.
list<DataNode>::const_iterator DataFile::begin() const
{
	return root.begin();
}



// Get an iterator to the end of the list of nodes in this file.
list<DataNode>::const_iterator DataFile::end() const
{
	return root.end();
}



// Parse the given text.
void DataFile::Load(const char *it, const char *end)
{
	// Keep track of the current stack of indentation levels and the most recent
	// node at each level - that is, the node that will be the "parent" of any
	// new node added at the next deeper indentation level.
	vector<DataNode *> stack(1, &root);
	vector<int> whiteStack(1, -1);
	bool fileIsSpaces = false;
	bool warned = false;
	size_t lineNumber = 0;
	
	for( ; it != end; ++it)
	{
		++lineNumber;
		// Find the first non-white character in this line.
		bool isSpaces = false;
		int white = 0;
		for( ; *it <= ' ' && *it != '\n'; ++it)
		{
			// Warn about mixed indentations when parsing files.
			if(!isSpaces && *it == ' ')
			{
				// If we've parsed whitespace that wasn't a space, issue a warning.
				if(white)
					stack.back()->PrintTrace("Mixed whitespace usage in line");
				else
					fileIsSpaces = true;
				
				isSpaces = true;
			}
			else if(fileIsSpaces && !warned && *it != ' ')
			{
				warned = true;
				stack.back()->PrintTrace("Mixed whitespace usage in file");
			}
			
			++white;
		}
		
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
		children.emplace_back(stack.back());
		DataNode &node = children.back();
		node.lineNumber = lineNumber;
		
		// Remember where in the tree we are.
		stack.push_back(&node);
		whiteStack.push_back(white);
		
		// Tokenize the line. Skip comments and empty lines.
		while(*it != '\n')
		{
			// Check if this token begins with a quotation mark. If so, it will
			// include everything up to the next instance of that mark.
			char endQuote = *it;
			bool isQuoted = (endQuote == '"' || endQuote == '`');
			it += isQuoted;
			
			const char *start = it;
			
			// Find the end of this token.
			while(*it != '\n' && (isQuoted ? (*it != endQuote) : (*it > ' ')))
				++it;
			
			// It ought to be legal to construct a string from an empty iterator
			// range, but it appears that some libraries do not handle that case
			// correctly. So:
			if(start == it)
				node.tokens.emplace_back();
			else
				node.tokens.emplace_back(start, it);
			// This is not a fatal error, but it may indicate a format mistake:
			if(isQuoted && *it == '\n')
				node.PrintTrace("Closing quotation mark is missing:");
			
			if(*it != '\n')
			{
				// If we've not yet reached the end of the line of text, search
				// forward for the next non-whitespace character.
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
