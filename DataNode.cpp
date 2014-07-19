/* DataNode.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DataNode.h"

#include <limits>
#include <sstream>

using namespace std;



int DataNode::Size() const
{
	return tokens.size();
}



const string &DataNode::Token(int index) const
{
	return tokens[index];
}



double DataNode::Value(int index) const
{
	double value = numeric_limits<double>::quiet_NaN();
	istringstream(tokens[index]) >> value;
	
	return value;
}



list<DataNode>::const_iterator DataNode::begin() const
{
	return children.begin();
}



list<DataNode>::const_iterator DataNode::end() const
{
	return children.end();
}
