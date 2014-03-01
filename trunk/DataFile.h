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

#include <istream>
#include <ostream>
#include <list>
#include <string>
#include <vector>



// A class which represents a hierarchical data file.
class DataFile {
public:
	DataFile();
	DataFile(const std::string &path);
	DataFile(std::istream &in);
	
	void Load(const std::string &path);
	void Load(std::istream &in);
	
	class Node;
	std::list<Node>::const_iterator begin() const;
	std::list<Node>::const_iterator end() const;
	
	
public:
	class Node {
	public:
		int Size() const;
		const std::string &Token(int index) const;
		double Value(int index) const;
		
		std::list<Node>::const_iterator begin() const;
		std::list<Node>::const_iterator end() const;
		
		void Write(std::ostream &out) const;
		
		
	private:
		std::string raw;
		std::list<Node> children;
		std::vector<std::string> tokens;
		
		friend class DataFile;
	};
	
	
private:
	Node root;
};



#endif
