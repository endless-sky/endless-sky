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



DataFile::DataFile()
{
}



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
	ifstream in(path);
	Load(in);
}



void DataFile::Load(istream &in)
{
	vector<DataNode *> stack(1, &root);
	vector<int> whiteStack(1, -1);
	
	string line;
	while(getline(in, line))
	{
		int length = line.length();
		
		int white = 0;
		while(white < length && line[white] <= ' ')
			++white;
		
		if(white == length || line[white] == '#')
			continue;
		while(whiteStack.back() >= white)
		{
			whiteStack.pop_back();
			stack.pop_back();
		}
		
		list<DataNode> &children = stack.back()->children;
		children.push_back(DataNode());
		DataNode &node = children.back();
		
		stack.push_back(&node);
		whiteStack.push_back(white);
		
		// Tokenize the line. Skip comments and empty lines.
		if(white != length && line[white] != '#')
		{
			int i = white;
			while(i != length)
			{
				char endQuote = line[i];
				bool isQuoted = (endQuote == '"' || endQuote == '`');
				i += isQuoted;
				
				node.tokens.push_back(string());
				while(i != length && (isQuoted ? (line[i] != endQuote) : (line[i] > ' ')))
					node.tokens.back() += line[i++];
				
				if(i != length)
				{
					i += isQuoted;
					while(i != length && line[i] <= ' ')
						++i;
				}
			}
		}
		
		children.back().raw.swap(line);
	}
}



list<DataNode>::const_iterator DataFile::begin() const
{
	return root.begin();
}



list<DataNode>::const_iterator DataFile::end() const
{
	return root.end();
}
