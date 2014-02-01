/* DataFile.cpp
Michael Zahniser, 14 Oct 2013

Function definitions for the DataFile class.
*/

#include "DataFile.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <limits>

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
	vector<Node *> stack(1, &root);
	vector<int> whiteStack(1, -1);
	
	string line;
	while(getline(in, line))
	{
		int white = 0;
		while(white < line.length() && line[white] <= ' ')
			++white;
		
		if(white == line.length() || line[white] == '#')
			continue;
		while(whiteStack.back() >= white)
		{
			whiteStack.pop_back();
			stack.pop_back();
		}
		
		list<Node> &children = stack.back()->children;
		children.push_back(Node());
		Node &node = children.back();
		
		stack.push_back(&node);
		whiteStack.push_back(white);
		
		// Tokenize the line. Skip comments and empty lines.
		if(white != line.length() && line[white] != '#')
		{
			int i = white;
			while(i != line.length())
			{
				bool isQuoted = (line[i] == '"');
				i += isQuoted;
				
				node.tokens.push_back(string());
				while(i != line.length() && (isQuoted ? (line[i] != '"') : (line[i] > ' ')))
					node.tokens.back() += line[i++];
				
				if(i != line.length())
				{
					i += isQuoted;
					while(i != line.length() && line[i] <= ' ')
						++i;
				}
			}
		}
		
		children.back().raw.swap(line);
	}
}



list<DataFile::Node>::const_iterator DataFile::begin() const
{
	return root.begin();
}



list<DataFile::Node>::const_iterator DataFile::end() const
{
	return root.end();
}



int DataFile::Node::Size() const
{
	return tokens.size();
}



const string &DataFile::Node::Token(int index) const
{
	assert(index >= 0 && index < tokens.size());
	
	return tokens[index];
}



double DataFile::Node::Value(int index) const
{
	assert(index >= 0 && index < tokens.size());
	
	double value = numeric_limits<double>::quiet_NaN();
	istringstream(tokens[index]) >> value;
	
	return value;
}



list<DataFile::Node>::const_iterator DataFile::Node::begin() const
{
	return children.begin();
}



list<DataFile::Node>::const_iterator DataFile::Node::end() const
{
	return children.end();
}



void DataFile::Node::Write(ostream &out) const
{
	out << raw << endl;
	for(const Node &node : children)
		node.Write(out);
}
