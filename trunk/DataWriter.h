#ifndef DATA_WRITER_H_
#define DATA_WRITER_H_

#include <string>
#include <fstream>



// This class writes data in a hierarchical format, where an indented line is
// considered the "child" of the first line above it that is less indented. By
// using this class, you can have a function add data to the file without having
// to tell that function what indentation level it is at. This class also
// automatically adds quotation marks around strings if they contain whitespace.
class DataWriter {
public:
	DataWriter(const std::string &path);
	
  template <class ...B>
	void Write(const char *a, B... others);
  template <class ...B>
	void Write(const std::string &a, B... others);
  template <class A, class ...B>
	void Write(const A &a, B... others);
	void Write();
	
	void BeginChild();
	void EndChild();
	
	void WriteComment(const std::string &str);
	
	
private:
	std::string indent;
	static const std::string space;
	const std::string *before;
	std::ofstream out;
};



template <class ...B>
void DataWriter::Write(const char *a, B... others)
{
	bool hasSpace = false;
	for(const char *it = a; *it; ++it)
		if(*it <= ' ')
		{
			hasSpace = true;
			break;
		}
	
	out << *before;
	if(hasSpace)
		out << '"' << a << '"';
	else
		out << a;
	before = &space;
	
	Write(others...);
}



template <class ...B>
void DataWriter::Write(const std::string &a, B... others)
{
	Write(a.c_str(), others...);
}



template <class A, class ...B>
void DataWriter::Write(const A &a, B... others)
{
	static_assert(std::is_arithmetic<A>::value,
		"DataWriter cannot output anything but strings and arithmetic types.");
	
	out << *before << a;
	before = &space;
	
	Write(others...);
}



#endif
