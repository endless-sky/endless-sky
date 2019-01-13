/* DataFile.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DATA_FILE_H_
#define DATA_FILE_H_

#include "DataNode.h"

#include <istream>
#include <list>
#include <string>



// A class which represents a hierarchical data file. Each line of the file that
// is not empty or a comment is a "node," and the relationship between the nodes
// is determined by indentation: if a node is more indented than the node before
// it, it is a "child" of that node. Otherwise, it is a "sibling." Each node is
// just a collection of one or more tokens that can be interpreted either as
// strings or as floating point values; see DataNode for more information.
class DataFile {
public:
	// A DataFile can be loaded either from a file path or an istream.
	DataFile() = default;
	explicit DataFile(const std::string &path);
	explicit DataFile(std::istream &in);
	
	void Load(const std::string &path);
	void Load(std::istream &in);
	
	// Functions for iterating through all DataNodes in this file.
	std::list<DataNode>::const_iterator begin() const;
	std::list<DataNode>::const_iterator end() const;
	
	
private:
	void LoadData(const std::string &data);
	
	
private:
	// This is the container for all DataNodes in this file.
	DataNode root;
};



#endif
