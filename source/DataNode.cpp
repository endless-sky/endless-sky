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

#include <cmath>
#include <limits>

using namespace std;



DataNode::DataNode()
{
	tokens.reserve(4);
}



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
	// Check for empty strings and out-of-bounds indices.
	if(static_cast<size_t>(index) >= tokens.size() || tokens[index].empty())
		return numeric_limits<double>::quiet_NaN();
	
	// Allowed format: "[+-]?[0-9]*[.]?[0-9]*([eE][+-]?[0-9]*)?".
	const char *it = tokens[index].c_str();
	if(*it != '-' && *it != '.' && *it != '+' && !(*it >= '0' && *it <= '9'))
		return numeric_limits<double>::quiet_NaN();
	
	// Check for leading sign.
	double sign = (*it == '-') ? -1. : 1.;
	it += (*it == '-' || *it == '+');
	
	// Digits before the decimal point.
	int64_t value = 0;
	while(*it >= '0' && *it <= '9')
		value = (value * 10) + (*it++ - '0');
	
	// Digits after the decimal point (if any).
	int64_t power = 0;
	if(*it == '.')
	{
		++it;
		while(*it >= '0' && *it <= '9')
		{
			value = (value * 10) + (*it++ - '0');
			--power;
		}
	}
	
	// Exponent.
	if(*it == 'e' || *it == 'E')
	{
		++it;
		int64_t sign = (*it == '-') ? -1 : 1;
		it += (*it == '-' || *it == '+');
		
		int64_t exponent = 0;
		while(*it >= '0' && *it <= '9')
			exponent = (exponent * 10) + (*it++ - '0');
		
		power += sign * exponent;
	}
	
	// Compose the return value.
	return copysign(value * pow(10., power), sign);
}



bool DataNode::HasChildren() const
{
	return !children.empty();
}



list<DataNode>::const_iterator DataNode::begin() const
{
	return children.begin();
}



list<DataNode>::const_iterator DataNode::end() const
{
	return children.end();
}
