/* test_categoryList.cpp
Copyright (c) 2022 by Amazinite

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
#include "../../../source/CategoryList.h"

// Include a helper for creating well-formed DataNodes (to enable creating non-empty CategoryLists).
#include "datanode-factory.h"

// ... and any system includes needed for the test file.
#include <algorithm>
#include <string>
#include <vector>

namespace { // test namespace

// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
SCENARIO( "Creating a CategoryList" , "[CategoryList][Creation]" ) {
	GIVEN( "a category list" ) {
		auto list = CategoryList{};
		// A manually sorted list to compare against.
		std::vector<std::string> sorted;
		// A helper function for determining if a string is equal to a Category's name.
		auto equal = [](const std::string &s, const CategoryList::Category &c) noexcept -> bool { return s == c.Name(); };

		WHEN( "sorted contents are given to the list without precedence" ) {
			list.Load(AsDataNode("category test\n\tfirst\n\tsecond\n\tthird"));
			sorted.push_back("first"); // Precedence = 0, as that is the default precedence of the first added category.
			sorted.push_back("second"); // Precedence = 1, as each new category uses the last used precedence + 1.
			sorted.push_back("third"); // Precedence = 2

			THEN( "the list is already sorted" ) {
				// sorted = first, second, third
				CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
			}

			THEN( "sorting the list does not change its ordering" ) {
				list.Sort();
				// sorted = first, second, third
				CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
			}
		}

		WHEN( "sorted contents are given to the list with precedence" ) {
			list.Load(AsDataNode("category test\n\tfirst 10\n\tsecond 20\n\tthird 30"));
			sorted.push_back("first"); // Precedence = 10
			sorted.push_back("second"); // Precedence = 20
			sorted.push_back("third"); // Precedence = 30

			THEN( "the list is already sorted" ) {
				// sorted = first, second, third
				CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
			}

			THEN( "sorting the list does not change its ordering" ) {
				list.Sort();
				// sorted = first, second, third
				CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
			}
		}

		WHEN( "unsorted contents are given to the list with precedence" ) {
			list.Load(AsDataNode("category test\n\tfirst 7\n\tsecond 2\n\tthird 4"));
			sorted.push_back("second"); // Precedence = 2
			sorted.push_back("third"); // Precedence = 4
			sorted.push_back("first"); // Precedence = 7

			THEN( "the list is unsorted" ) {
				// sorted = second, third, first
				CHECK_FALSE( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
			}

			THEN( "sorting the list correctly changes its ordering" ) {
				list.Sort();
				// sorted = second, third, first
				CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );

				AND_WHEN( "a list is loaded again without precedence" ) {
					list.Load(AsDataNode("category test\n\tfourth\n\tfifth"));

					THEN( "the new categories are at the end of the list in the order they were added" ) {
						sorted.push_back("fourth"); // Precedence = 5, as the last used precedence was 4.
						sorted.push_back("fifth"); // Precedence = 6
						// sorted = second, third, first, fourth, fifth
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}

					THEN( "sorting the list moves the new categories into the correct positions" ) {
						sorted[2] = "fourth"; // Precedence = 5, as the last used precedence was 4.
						sorted.push_back("fifth"); // Precedence = 6
						sorted.push_back("first"); // Precedence = 7
						// sorted = second, third, fourth, fifth, first
						list.Sort();
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}
				}

				AND_WHEN( "a list is loaded again with precedence" ) {
					list.Load(AsDataNode("category test\n\tfourth 1\n\tfifth 3"));

					THEN( "the new categories are at the end of the list in the order they were added" ) {
						sorted.push_back("fourth"); // Precedence = 1
						sorted.push_back("fifth"); // Precedence = 3
						// sorted = second, third, first, fourth, fifth
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}

					THEN( "sorting the list moves the new categories into the correct positions" ) {
						sorted[0] = "fourth"; // Precedence = 1
						sorted[1] = "second"; // Precedence = 2
						sorted[2] = "fifth"; // Precedence = 3
						sorted.push_back("third"); // Precedence = 4
						sorted.push_back("first"); // Precedence = 7
						list.Sort();
						// sorted = fourth, second, fifth, third, first
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}
				}

				AND_WHEN( "a list is loaded again with two categories of the same precedence" ) {
					list.Load(AsDataNode("category test\n\tfourth 7\n\tfifth 7"));

					THEN( "the new categories are at the end of the list in the order they were added" ) {
						sorted.push_back("fourth"); // Precedence = 7
						sorted.push_back("fifth"); // Precedence = 7
						// sorted = second, third, first, fourth, fifth
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}

					THEN( "after sorting, the categories with the same precedence become alphabetically ordered" ) {
						sorted[2] = "fifth"; // Precedence = 7
						sorted.push_back("first"); // Precedence = 7
						sorted.push_back("fourth"); // Precedence = 7
						list.Sort();
						// sorted = second, third, fifth, first, fourth
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}
				}

				AND_WHEN( "a list is loaded again with a category that already exists but with a different precedence" ) {
					list.Load(AsDataNode("category test\n\tthird 1"));

					THEN( "the duplicate category's position is unchanged but its precedence is updated" ) {
						// sorted = second, third, first
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}

					THEN( "after sorting, the duplicate category's position is corrected" ) {
						sorted[0] = "third"; // Precedence = 1
						sorted[1] = "second"; // Precedence = 2
						sorted[2] = "first"; // Precedence = 6
						list.Sort();
						// sorted = third, second, first
						CHECK( std::equal(sorted.begin(), sorted.end(), list.begin(), equal) );
					}
				}
			}
		}
	}
}
// #endregion unit tests



} // test namespace
