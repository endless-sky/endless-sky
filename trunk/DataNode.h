/* DataNode.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DATA_NODE_H_
#define DATA_NODE_H_

#include <list>
#include <string>
#include <vector>



class DataNode {
public:
	int Size() const;
	const std::string &Token(int index) const;
	double Value(int index) const;
	
	std::list<DataNode>::const_iterator begin() const;
	std::list<DataNode>::const_iterator end() const;
	
	
private:
	std::string raw;
	std::list<DataNode> children;
	std::vector<std::string> tokens;
	
	friend class DataFile;
};



#endif
