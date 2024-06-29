/* test_datawriter.cpp
Copyright (c) 2024 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/DataWriter.h"

// ... and any system includes needed for the test file.
#include "../../../source/DataNode.h"

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "DataWriter::Quote", "[datawriter][quote]" ) {
	CHECK( DataWriter::Quote("") == "\"\"" );
	CHECK( DataWriter::Quote(" ") == "\" \"" );
	CHECK( DataWriter::Quote("a") == "a" );
	CHECK( DataWriter::Quote("multiple spaces here ") == "\"multiple spaces here \"" );
	CHECK( DataWriter::Quote("\"") == "`\"`" );
	CHECK( DataWriter::Quote("quote and\" space ") == "`quote and\" space `" );
	CHECK( DataWriter::Quote("`") == "\"`\"" );
	CHECK( DataWriter::Quote("long ` text") == "\"long ` text\"" );
}

TEST_CASE( "DataWriter::WriteComment", "[datawriter][writecomment]" ) {
	DataWriter writer;
	GIVEN( "an empty DataWriter" ) {
		THEN( "writing a comment is possible" ) {
			writer.WriteComment("hello");
			CHECK( writer.SaveToString() == "# hello\n" );
		}
	}
	GIVEN( "a DataWriter with a partial line" ) {
		writer.WriteToken("hello there");
		THEN( "writing a comment is possible" ) {
			writer.WriteComment("comment");
			writer.Write("next line");
			CHECK( writer.SaveToString() == "\"hello there\" # comment\n\"next line\"\n" );
		}
	}
	GIVEN( "a DataWriter with multiple lines" ) {
		writer.Write("hello", "there");
		THEN( "writing a comment is possible" ) {
			writer.WriteComment("comment");
			CHECK( writer.SaveToString() == "hello there\n# comment\n" );
		}
	}
	GIVEN( "a DataWriter with indentation" ) {
		writer.Write("hello");
		writer.BeginChild();
		THEN( "writing a comment is possible" ) {
			writer.Write("there");
			writer.WriteComment("comment");
			writer.Write("after comment");
			writer.EndChild();
			writer.Write("outer");
			CHECK( writer.SaveToString() == "hello\n\tthere\n\t# comment\n\t\"after comment\"\nouter\n" );
		}
		THEN( "writing an inline comment is possible" ) {
			writer.WriteToken("there");
			writer.WriteComment("comment");
			writer.EndChild();
			CHECK( writer.SaveToString() == "hello\n\tthere # comment\n" );
		}
	}
	GIVEN( "a DataWriter with multiple levels of indentation" ) {
		writer.Write("first");
		writer.BeginChild();
		writer.Write("second");
		writer.BeginChild();
		writer.Write("third");
		THEN( "writing a comment is possible" ) {
			writer.WriteComment("comment");
			writer.Write("after comment");
			writer.EndChild();
			writer.Write("second after");
			CHECK( writer.SaveToString() ==
R"(first
	second
		third
		# comment
		"after comment"
	"second after"
)");
		}
		THEN( "writing an inline comment is possible" ) {
			writer.WriteToken("begin");
			writer.WriteComment("comment");
			writer.EndChild();
			writer.Write("second after");
			CHECK( writer.SaveToString() ==
R"(first
	second
		third
		begin # comment
	"second after"
)");
		}
	}
}

TEST_CASE( "DataWriter::WriteSorted", "[datawriter][writesorted]" ) {
	GIVEN( "a DataWriter" ) {
		DataWriter writer;
		std::map<const std::string *, double> data;
		using InnerType = std::pair<const std::string * const, double>;
		const auto &sortF = [&](const InnerType *a, const InnerType *b) {return a->second < b->second;};
		const auto &writeF = [&](const InnerType &it){writer.Write(*it.first);};
		AND_GIVEN( "no data" ) {
			THEN( "sorting is possible" ) {
				WriteSorted(data, sortF, writeF);
				CHECK( writer.SaveToString() == "" );
			}
		}
		AND_GIVEN( "a single data point" ) {
			std::string buffer = "1";
			data.emplace(&buffer, 8);
			THEN( "sorting is possible" ) {
				WriteSorted(data, sortF, writeF);
				CHECK( writer.SaveToString() == "1\n" );
			}
		}
		AND_GIVEN( "multiple data points" ) {
			std::string buffer[] = {"1", "6", "3", "4", "5", "2"};
			double nums[] = {1, 6, 3, 4, 5, 2};
			for(int i = 0; i < 6; i++)
				data.emplace(buffer + i, nums[i]);

			THEN( "sorting is possible" ) {
				WriteSorted(data, sortF, writeF);
				CHECK( writer.SaveToString() == "1\n2\n3\n4\n5\n6\n" );
			}
		}
	}
}

TEST_CASE( "DataWriter::Write", "[datawriter][write]" ) {
	GIVEN( "a DataWriter" ) {
		DataWriter writer;
		DataNode node;
		AND_GIVEN( "a single-level node" ) {
			node.AddToken("first");
			node.AddToken("line");
			THEN( "the node can be written" ) {
				writer.Write(node);
				CHECK( writer.SaveToString() == "first line\n" );
			}
		}
		AND_GIVEN( "a multi-level node" ) {
			node.AddToken("first");
			node.AddToken("line");
			DataNode child(&node);
			child.AddToken("second");
			child.AddToken("line");
			DataNode child2(&node);
			child2.AddToken("third");
			DataNode child3(&child);
			child3.AddToken("inner");
			child.AddChild(child3);
			node.AddChild(child);
			node.AddChild(child2);
			THEN( "the node can be written" ) {
				writer.Write(node);
				CHECK( writer.SaveToString() == "first line\n\tsecond line\n\t\tinner\n\tthird\n" );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
