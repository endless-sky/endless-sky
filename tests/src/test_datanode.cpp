/* test_datanode.cpp
Copyright (c) 2020 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/DataNode.h"

// ... and any system includes needed for the test file.
#include <string>
#include <vector>

namespace { // test namespace
// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
SCENARIO( "Creating a DataNode", "[DataNode]") {
	const DataNode root;
	GIVEN( "When created" ) {
		THEN( "it has the correct default properties" ) {
			CHECK( root.Size() == 0 );
			CHECK_FALSE( root.HasChildren() );
			CHECK( root.Tokens().empty() );
		}
		THEN( "it preallocates capacity for tokens" ) {
			CHECK( root.Tokens().capacity() == 4 );
		}
	}
	GIVEN( "When created without a parent" ) {
		THEN( "it prints its token trace at the correct level" ) {
			CHECK( root.PrintTrace() == 0 );
		}
	}
	GIVEN( "When created with a parent" ) {
		const DataNode child(&root);
		THEN( "it still has the correct default properties" ) {
			CHECK( child.Size() == 0 );
			CHECK_FALSE( child.HasChildren() );
			CHECK( child.Tokens().empty() );
		}
		THEN( "it prints its token trace at the correct level" ) {
			CHECK( child.PrintTrace() == 2 );
		}
		THEN( "no automatic registration with the parent is done" ) {
			CHECK_FALSE( root.HasChildren() );
		}
	}
}

SCENARIO( "Determining if a token is numeric", "[IsNumber][Parsing][DataNode]" ) {
	GIVEN( "An integer string" ) {
		THEN( "IsNumber returns true" ) {
			auto strNum = GENERATE(as<std::string>{}
				, "1"
				, "10"
				, "100"
				, "1000000000000000"
			);
			CAPTURE( strNum ); // Log the value if the assertion fails.
			CHECK( DataNode::IsNumber(strNum) );
		}
	}
}
// #endregion unit tests



} // test namespace
