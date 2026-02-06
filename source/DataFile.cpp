/* DataFile.cpp
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

#include "DataFile.h"

#include "Files.h"
#include "text/Utf8.h"

using namespace std;



// Constructor, taking a file path (in UTF-8).
DataFile::DataFile(const filesystem::path &path)
{
	Load(path);
}



// Constructor, taking an istream. This can be cin or a file.
DataFile::DataFile(istream &in)
{
	Load(in);
}



// Load from a file path (in UTF-8).
void DataFile::Load(const filesystem::path &path)
{
	string data = Files::Read(path);
	if(data.empty())
		return;

	// As a sentinel, make sure the file always ends in a newline.
	if(data.back() != '\n')
		data.push_back('\n');

	// Note what file this node is in, so it will show up in error traces.
	root.tokens.push_back("file");
	root.tokens.push_back(path.string());

	LoadData(data);
}



// Constructor, taking an istream. This can be cin or a file.
void DataFile::Load(istream &in)
{
	string data;

	static const size_t BLOCK = 4096;
	while(in)
	{
		size_t currentSize = data.size();
		data.resize(currentSize + BLOCK);
		in.read(&*data.begin() + currentSize, BLOCK);
		data.resize(currentSize + in.gcount());
	}
	// As a sentinel, make sure the file always ends in a newline.
	if(data.empty() || data.back() != '\n')
		data.push_back('\n');

	LoadData(data);
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
void DataFile::LoadData(const string &data)
{
	// Keep track of the current stack of indentation levels and the most recent
	// node at each level - that is, the node that will be the "parent" of any
	// new node added at the next deeper indentation level.
	vector<DataNode *> stack(1, &root);
	vector<int> separatorStack(1, -1);
	bool fileIsTabs = false;
	bool fileIsSpaces = false;
	size_t lineNumber = 0;

	size_t end = data.length();

	size_t pos = 0;
	// If the first character is the UTF8 byte order mark (BOM), skip it.
	if(!Utf8::IsBOM(Utf8::DecodeCodePoint(data, pos)))
		pos = 0;

	while(pos < end)
	{
		++lineNumber;
		size_t tokenPos = pos;
		char32_t c = Utf8::DecodeCodePoint(data, pos);

		bool mixedIndentation = false;
		int separators = 0;
		// Find the first tokenizable character in this line (i.e. neither space nor tab).
		while(c <= ' ' && c != '\n')
		{
			// Determine what type of indentation this file is using.
			if(!fileIsTabs && !fileIsSpaces)
			{
				if(c == '\t')
					fileIsTabs = true;
				else if(c == ' ')
					fileIsSpaces = true;
			}
			// Issue a warning if the wrong indentation is used.
			else if((fileIsTabs && c != '\t') || (fileIsSpaces && c != ' '))
				mixedIndentation = true;

			++separators;
			tokenPos = pos;
			c = Utf8::DecodeCodePoint(data, pos);
		}

		// If the line is a comment, skip to the end of the line.
		if(c == '#')
		{
			if(mixedIndentation)
				root.PrintTrace("Mixed whitespace usage for comment at line " + to_string(lineNumber));
			while(c != '\n')
				c = Utf8::DecodeCodePoint(data, pos);
		}
		// Skip empty lines (including comment lines).
		if(c == '\n')
			continue;

		// Determine where in the node tree we are inserting this node, based on
		// whether it has more indentation that the previous node, less, or the same.
		while(separatorStack.back() >= separators)
		{
			separatorStack.pop_back();
			stack.pop_back();
		}

		// Add this node as a child of the proper node.
		list<DataNode> &children = stack.back()->children;
		children.emplace_back(stack.back());
		DataNode &node = children.back();
		node.lineNumber = lineNumber;

		// Remember where in the tree we are.
		stack.push_back(&node);
		separatorStack.push_back(separators);

		// Tokenize the line. Skip comments and empty lines.
		while(c != '\n')
		{
			// Check if this token begins with a quotation mark. If so, it will
			// include everything up to the next instance of that mark.
			char32_t endQuote = c;
			bool isQuoted = (endQuote == '"' || endQuote == '`');
			if(isQuoted)
			{
				tokenPos = pos;
				c = Utf8::DecodeCodePoint(data, pos);
			}

			size_t endPos = tokenPos;

			// Find the end of this token.
			while(c != '\n' && (isQuoted ? (c != endQuote) : (c > ' ')))
			{
				endPos = pos;
				c = Utf8::DecodeCodePoint(data, pos);
			}

			// It ought to be legal to construct a string from an empty iterator
			// range, but it appears that some libraries do not handle that case
			// correctly. So:
			if(tokenPos == endPos)
				node.tokens.emplace_back();
			else
				node.tokens.emplace_back(data, tokenPos, endPos - tokenPos);
			// This is not a fatal error, but it may indicate a format mistake:
			if(isQuoted && c == '\n')
				node.PrintTrace("Closing quotation mark is missing:");

			if(c != '\n')
			{
				// If we've not yet reached the end of the line of text, search
				// forward for the next non-whitespace character.
				if(isQuoted)
				{
					tokenPos = pos;
					c = Utf8::DecodeCodePoint(data, pos);
				}
				while(c != '\n' && c <= ' ' && c != '#')
				{
					tokenPos = pos;
					c = Utf8::DecodeCodePoint(data, pos);
				}

				// If a comment is encountered outside of a token, skip the rest
				// of this line of the file.
				if(c == '#')
				{
					while(c != '\n')
						c = Utf8::DecodeCodePoint(data, pos);
				}
			}
		}
		// Now that we've reached the end of the line, we know no more tokens will be added to the node.
		node.tokens.shrink_to_fit();

		// Now that we've tokenized this node, print any mixed whitespace warnings.
		if(mixedIndentation)
			node.PrintTrace("Mixed whitespace usage at line");
	}
}
