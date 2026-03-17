/* test_datanode.cpp
Copyright (c) 2020 by Benjamin Hauch

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
#include "../../../source/DataNode.h"

// Include a helper for creating well-formed DataNodes.
#include "datanode-factory.h"
#include "logger-output.h"
#include "output-capture.hpp"

// ... and any system includes needed for the test file.
#include <string>
#include <vector>

namespace { // test namespace
// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
TEST_CASE( "DataNode Basics", "[DataNode]" ) {
	using T = DataNode;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial_v<T> );
		// The class layout apparently satisfies StandardLayoutType when building/testing for Steam, but false otherwise.
		// This may change in the future, with the expectation of false everywhere (due to the list<DataNode> field).
		// CHECK_FALSE( std::is_standard_layout_v<T> );
		CHECK( std::is_nothrow_destructible_v<T> );
		CHECK_FALSE( std::is_trivially_destructible_v<T> );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible_v<T> );
		CHECK_FALSE( std::is_trivially_default_constructible_v<T> );
		// We explicitly allocate some memory when default-constructing DataNodes.
		CHECK_FALSE( std::is_nothrow_default_constructible_v<T> );
		CHECK( std::is_copy_constructible_v<T> );
		// We have work to do when copy-constructing, including allocations.
		CHECK_FALSE( std::is_trivially_copy_constructible_v<T> );
		CHECK_FALSE( std::is_nothrow_copy_constructible_v<T> );
		CHECK( std::is_move_constructible_v<T> );
		// We have work to do when move-constructing.
		CHECK_FALSE( std::is_trivially_move_constructible_v<T> );
		CHECK( std::is_nothrow_move_constructible_v<T> );
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable_v<T> );
		// The class data can be spread out due to vector + list contents.
		CHECK_FALSE( std::is_trivially_copyable_v<T> );
		// We have work to do when copying.
		CHECK_FALSE( std::is_trivially_copy_assignable_v<T> );
		CHECK_FALSE( std::is_nothrow_copy_assignable_v<T> );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable_v<T> );
		CHECK_FALSE( std::is_trivially_move_assignable_v<T> );
		CHECK( std::is_nothrow_move_assignable_v<T> );
	}
}

SCENARIO( "Creating a DataNode", "[DataNode]") {
	OutputSink traces(std::cerr);
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
	GIVEN( "A DataNode with child nodes" ) {
		DataNode parent = AsDataNode("parent\n\tchild\n\t\tgrand");
		WHEN( "Copying by assignment" ) {
			DataNode partner;
			partner = parent;
			parent = DataNode();
			THEN( "the child nodes are copied" ) {
				REQUIRE( partner.HasChildren() );
				const DataNode &child = *partner.begin();
				REQUIRE( child.HasChildren() );
				const DataNode &grand = *child.begin();
				CHECK_FALSE( grand.HasChildren() );

				AND_THEN( "The copied children have the correct tokens" ) {
					REQUIRE( partner.Size() == 1 );
					CHECK( partner.Token(0) == "parent" );
					REQUIRE( child.Size() == 1 );
					CHECK( child.Token(0) == "child" );
					REQUIRE( grand.Size() == 1 );
					CHECK( grand.Token(0) == "grand" );
				}

				AND_THEN( "The copied children print correct traces" ) {
					CHECK( partner.PrintTrace() == 0 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\n" );
					CHECK( child.PrintTrace() == 2 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\n");
					CHECK( grand.PrintTrace() == 4 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\nL3:     grand\n");
				}
			}
		}
		WHEN( "Copy-constructing" ) {
			DataNode partner(parent);
			parent = DataNode();
			THEN( "the children are copied" ) {
				REQUIRE( partner.HasChildren() );
				const DataNode &child = *partner.begin();
				REQUIRE( child.HasChildren() );
				const DataNode &grand = *child.begin();
				CHECK_FALSE( grand.HasChildren() );

				AND_THEN( "The copied children have the correct tokens" ) {
					REQUIRE( partner.Size() == 1 );
					CHECK( partner.Token(0) == "parent" );
					REQUIRE( child.Size() == 1 );
					CHECK( child.Token(0) == "child" );
					REQUIRE( grand.Size() == 1 );
					CHECK( grand.Token(0) == "grand" );
				}

				AND_THEN( "The copied children print correct traces" ) {
					CHECK( partner.PrintTrace() == 0 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\n" );
					CHECK( child.PrintTrace() == 2 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\n");
					CHECK( grand.PrintTrace() == 4 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\nL3:     grand\n");
				}
			}
		}
		WHEN( "Transferring via move assignment" ) {
			DataNode moved;
			moved = std::move(parent);
			parent = DataNode();
			THEN( "the children are moved" ) {
				REQUIRE( moved.HasChildren() );
				const DataNode &child = *moved.begin();
				REQUIRE( child.HasChildren() );
				const DataNode &grand = *child.begin();
				CHECK_FALSE( grand.HasChildren() );

				AND_THEN( "The moved children have the correct tokens" ) {
					REQUIRE( moved.Size() == 1 );
					CHECK( moved.Token(0) == "parent" );
					REQUIRE( child.Size() == 1 );
					CHECK( child.Token(0) == "child" );
					REQUIRE( grand.Size() == 1 );
					CHECK( grand.Token(0) == "grand" );
				}

				AND_THEN( "The moved children print correct traces" ) {
					CHECK( moved.PrintTrace() == 0 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\n" );
					CHECK( child.PrintTrace() == 2 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\n");
					CHECK( grand.PrintTrace() == 4 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\nL3:     grand\n");
				}
			}
		}
		WHEN( "Transferring via move construction" ) {
			DataNode moved(std::move(parent));
			parent = DataNode();
			THEN( "the children are moved" ) {
				REQUIRE( moved.HasChildren() );
				const DataNode &child = *moved.begin();
				REQUIRE( child.HasChildren() );
				const DataNode &grand = *child.begin();
				CHECK_FALSE( grand.HasChildren() );

				AND_THEN( "The moved children have the correct tokens" ) {
					REQUIRE( moved.Size() == 1 );
					CHECK( moved.Token(0) == "parent" );
					REQUIRE( child.Size() == 1 );
					CHECK( child.Token(0) == "child" );
					REQUIRE( grand.Size() == 1 );
					CHECK( grand.Token(0) == "grand" );
				}

				AND_THEN( "The moved children print correct traces" ) {
					CHECK( moved.PrintTrace() == 0 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\n" );
					CHECK( child.PrintTrace() == 2 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\n");
					CHECK( grand.PrintTrace() == 4 );
					CHECK( IgnoreLogHeaders(traces.Flush()) == "parent\nL2:   child\nL3:     grand\n");
				}
			}
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

SCENARIO( "Determining if a token is a boolean", "[Boolean][Parsing][DataNode]" ) {
	GIVEN( "A string that is \"true\"/\"1\" or \"false\"/\"0\"" ) {
		THEN( "IsBool returns true" ) {
			CHECK( DataNode::IsBool("true") );
			CHECK( DataNode::IsBool("1") );
			CHECK( DataNode::IsBool("false") );
			CHECK( DataNode::IsBool("0") );
		}
	}
	GIVEN( "A string that is not \"true\"/\"1\" or \"false\"/\"0\"" ) {
		THEN( "IsBool returns false" ) {
			CHECK_FALSE( DataNode::IsBool("monkey") );
			CHECK_FALSE( DataNode::IsBool("banana") );
			CHECK_FALSE( DataNode::IsBool("-1") );
			CHECK_FALSE( DataNode::IsBool("2") );
		}
	}
	GIVEN( "A DataNode with a boolean string token" ) {
		DataNode root = AsDataNode("root\n\ttrue\n\t\tfalse");
		const DataNode &trueVal = *root.begin();
		const DataNode &falseVal = *trueVal.begin();
		THEN( "BoolValue returns expected contents" ) {
			CHECK( trueVal.BoolValue(0) );
			CHECK_FALSE( falseVal.BoolValue(0) );
		}
	}
	GIVEN( "A DataNode with a boolean number token" ) {
		DataNode root = AsDataNode("root\n\t1\n\t\t0");
		const DataNode &trueVal = *root.begin();
		const DataNode &falseVal = *trueVal.begin();
		THEN( "BoolValue returns expected contents" ) {
			CHECK( trueVal.BoolValue(0) );
			CHECK_FALSE( falseVal.BoolValue(0) );
		}
	}
}
// #endregion unit tests



} // test namespace
