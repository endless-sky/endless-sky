/* test_datafile.cpp
Copyright (c) 2022 by quyykk

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
#include "../../../source/DataFile.h"

// Include a helper functions.
#include "datanode-factory.h"
#include "../../../source/text/Format.h"
#include "logger-output.h"
#include "output-capture.hpp"

// ... and any system includes needed for the test file.
#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace { // test namespace
// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.
const std::string missingQuoteWarning = "Closing quotation mark is missing:";
const std::string mixedCommentWarning = "Mixed whitespace usage for comment at line";
const std::string mixedNodeWarning = "Mixed whitespace usage at line";

std::vector<std::string> Split(const std::string &str)
{
	auto result = Format::Split(str, "\n");

	// Strip any empty lines from the output.
	result.erase(std::remove_if(result.begin(), result.end(), [](const std::string &val)
		{
			return val.empty();
		}), result.end());

	return result;
}

// #endregion mock data



// #region unit tests
SCENARIO( "Creating a DataFile", "[DataFile]") {
	GIVEN( "A default constructed DataFile" ) {
		const DataFile root;
		THEN( "it has the correct default properties" ) {
			CHECK( root.begin() == root.end() );
		}
	}
	GIVEN( "A DataFile created with a stream" ) {
		std::istringstream stream(R"(
node1
	foo

# parent comment

node2 hi
	something else
		# comment
		grand child
	# another comment
)");
		const DataFile root(stream);

		THEN( "iterating visits each node that has no indentation prefix" ) {
			REQUIRE( std::distance(root.begin(), root.end()) == 2 );
			CHECK( root.begin()->Token(0) == "node1" );
			CHECK( std::next(root.begin())->Token(0) == "node2" );
		}
		AND_THEN( "iterating parent nodes visits the child nodes" ) {
			std::set<std::string> children{"foo", "something"};
			for(const auto &parent : root)
				for(const auto &child : parent)
					REQUIRE(children.contains(child.Token(0)));
		}
		AND_THEN( "iterating child nodes visits their child nodes" ) {
			for(const auto &parent : root)
				for(const auto &child : parent)
					for(const auto &grand : child)
					{
						REQUIRE(grand.Token(0) == "grand");
						REQUIRE(grand.Token(1) == "child");
					}
		}
	}
}

SCENARIO( "Loading a DataFile with missing quotes", "[DataFile]" ) {
	OutputSink sink(std::cerr);

	GIVEN( "A leading quote at root level" ) {
		std::istringstream stream(R"("system Sol)");
		const DataFile root(stream);

		THEN( "A warning is issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			REQUIRE( warnings.size() == 2 );
			CHECK( warnings[0] == missingQuoteWarning );
			CHECK( warnings[1].find("system Sol") != std::string::npos );
		}
	}

	GIVEN( "A leading quote at child level" ) {
		std::istringstream stream(R"(
system Test
	something "else
)");
		const DataFile root(stream);

		THEN( "A warning is issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			REQUIRE( warnings.size() == 3 );
			CHECK( warnings[0] == missingQuoteWarning );
			CHECK( warnings[1].find("system Test") != std::string::npos );
			CHECK( warnings[2].find("something else") != std::string::npos );
		}
	}

	GIVEN( "A trailing quote" ) {
		std::istringstream stream(R"(
system" f
	this is" ok"
)");
		const DataFile root(stream);

		THEN( "No warnings are issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			CHECK( warnings.empty() );
		}
	}

	GIVEN( "Quotes in comments" ) {
		std::istringstream stream(R"(# system "foo)");
		const DataFile root(stream);

		THEN( "No warnings are issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			CHECK( warnings.empty() );
		}
	}
}

SCENARIO( "Loading a DataFile with mixed whitespace", "[DataFile]") {
	OutputSink sink(std::cerr);

	GIVEN( "Only tabs" ) {
		std::istringstream stream(R"(
system foo
	description bar
		no error
)");
		const DataFile root(stream);

		THEN( "No warnings are issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			CHECK( warnings.empty() );
		}
	}

	GIVEN( "Only spaces") {
		std::istringstream stream(R"(
system foo
 description bar
  no error
)");
		const DataFile root(stream);

		THEN( "No warnings are issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			CHECK( warnings.empty() );
		}
	}

	GIVEN( "Tabs and later spaces" ) {
		std::istringstream stream(R"(
system foo
	something

now with
 spaces
)");
		const DataFile root(stream);

		THEN( "A warning is issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			REQUIRE( warnings.size() == 3 );
			CHECK( warnings[0] == mixedNodeWarning );
			CHECK( warnings[1].find("now with") != std::string::npos );
			CHECK( warnings[2].find("spaces") != std::string::npos );
		}
	}

	GIVEN( "Spaces and later tabs" ) {
		std::istringstream stream(R"(
system foo
 something

now with
	tabs
)");
		const DataFile root(stream);

		THEN( "A warning is issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			REQUIRE( warnings.size() == 3 );
			CHECK( warnings[0] == mixedNodeWarning );
			CHECK( warnings[1].find("now with") != std::string::npos );
			CHECK( warnings[2].find("tabs") != std::string::npos );
		}
	}

	GIVEN( "Spaces and tabs on the same line" ) {
		std::istringstream stream(R"(
system test
	 foo
)");
		const DataFile root(stream);

		THEN( "A warning is issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			REQUIRE( warnings.size() == 3 );
			CHECK( warnings[0] == mixedNodeWarning );
			CHECK( warnings[1].find("system test") != std::string::npos );
			CHECK( warnings[2].find("foo") != std::string::npos );
		}
	}

	GIVEN( "Tabs and later spaces for comments" ) {
		std::istringstream stream(R"(
system foo
	# something

now with
 # spaces
)");
		const DataFile root(stream);

		THEN( "A warning is issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			REQUIRE( warnings.size() == 1 );
			CHECK( warnings[0] == mixedCommentWarning + " 6" );
		}
	}

	GIVEN( "Spaces and later tabs for comments" ) {
		std::istringstream stream(R"(
system foo
 # something

now with
	# tabs
)");
		const DataFile root(stream);

		THEN( "A warning is issued" ) {
			const auto warnings = Split(IgnoreLogHeaders(sink.Flush()));

			REQUIRE( warnings.size() == 1 );
			CHECK( warnings[0] == mixedCommentWarning + " 6" );
		}
	}
}
// #endregion unit tests



} // test namespace
