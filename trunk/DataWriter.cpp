#include "DataWriter.h"

using namespace std;



const std::string DataWriter::space = " ";



DataWriter::DataWriter(const string &path)
	: before(&indent)
{
	out.open(path);
	out.precision(8);
}



void DataWriter::Write()
{
	out << '\n';
	before = &indent;
}



void DataWriter::BeginChild()
{
	indent += '\t';
}



void DataWriter::EndChild()
{
	indent.erase(indent.length() - 1);
}



void DataWriter::WriteComment(const string &str)
{
	out << indent << "# " << str << '\n';
}
