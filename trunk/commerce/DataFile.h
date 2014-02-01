/* DataFile.h
Michael Zahniser, 14 Oct 2013

A class which represents a hierarchical data file.
*/

#ifndef DATA_FILE_H
#define DATA_FILE_H

#include <istream>
#include <ostream>
#include <list>
#include <string>
#include <vector>



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
