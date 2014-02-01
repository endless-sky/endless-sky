/* ShipName.cpp
Michael Zahniser, 5 Jan 2014

Function definitions for the ShipName class.
*/

#include "ShipName.h"

#include <cstdlib>

using namespace std;



void ShipName::Load(const DataFile::Node &node)
{
	if(node.Token(0) != "shipName")
		return;
	
	words.push_back(vector<vector<string>>());
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "word")
		{
			words.back().push_back(vector<string>());
			for(const DataFile::Node &grand : child)
				words.back().back().push_back(grand.Token(0));
		}
	}
}



string ShipName::Get() const
{
	string result;
	if(words.empty())
		return result;
	
	for(const vector<string> &v : words[rand() % words.size()])
		result += v[rand() % v.size()];
	
	return result;
}
