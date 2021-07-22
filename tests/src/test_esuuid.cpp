/* test_esuuid.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"
#include "output-capture.hpp"

// Include only the tested class's header.
#include "../../source/EsUuid.h"
// Declare the existence of the class internals that will be tested.
namespace es_uuid {
namespace detail {
	EsUuid::UuidType MakeUuid();
}
}

// ... and any system includes needed for the test file.
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

// Provide a string conversion for failing tests.
namespace Catch {
	template<>
	struct StringMaker<EsUuid> {
		static std::string convert(const EsUuid &value ) {
			return value.ToString();
		}
	};
}

namespace { // test namespace

// #region mock data
struct Identifiable {
	EsUuid id;
	const EsUuid &UUID() const noexcept { return id; }
};
// #endregion mock data

auto AsStrings = [](const std::vector<EsUuid> &container) -> std::vector<std::string> {
	auto strings = std::vector<std::string>(container.size());
	std::transform(container.begin(), container.end(), strings.begin(),
		[](const EsUuid &v) { return v.ToString(); });
	return strings;
};

// #region unit tests
TEST_CASE( "EsUuid class", "[uuid]" ) {
	using T = EsUuid;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial<T>::value );
		CHECK( std::is_standard_layout<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		CHECK( std::is_trivially_destructible<T>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK( std::is_nothrow_default_constructible<T>::value );
		// TODO: enable after refactoring how we create ships from stock models.
		// CHECK_FALSE( std::is_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		CHECK( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
	}
	// TODO: enable, as above.
	// SECTION( "Copy Traits" ) {
	// 	CHECK_FALSE( std::is_copy_assignable<T>::value );
	// }
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		CHECK( std::is_trivially_move_assignable<T>::value );
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}
}

SCENARIO( "Creating a UUID", "[uuid][creation]") {
	GIVEN( "No arguments" ) {
		EsUuid id;
		THEN( "it takes a random value" ) {
			CHECK_FALSE( id.ToString().empty() );
		}
	}
	GIVEN( "a reference string" ) {
		WHEN( "the string is valid" ) {
			const std::string valid = "5be91256-f6ba-47cd-96df-1ce1cb4fee86";
			THEN( "it takes the given value" ) {
				auto id = EsUuid::FromString(valid);
				CHECK( id.ToString() == valid );
			}
		}
		WHEN( "the string is invalid" ) {
			OutputSink warnings(std::cerr);
			THEN( "it logs warning messages" ) {
				auto invalid = GENERATE(as<std::string>{}
					, "abcdef"
					, "ZZZZZZZZ-ZZZZ-ZZZZ-ZZZZ-ZZZZZZZZZZZZ"
					, "5be91256-f6ba-47cd-96df-1ce1cb-fee86"
				);
				auto id = EsUuid::FromString(invalid);
				auto expected = "Cannot convert \"" + invalid + "\" into a UUID\n";
				CHECK( warnings.Flush() == expected );
				AND_THEN( "creates a random-valued ID" ) {
					CHECK( id.ToString() != invalid );
				}
			}
		}
	}
}

SCENARIO( "Comparing IDs", "[uuid][comparison]" ) {
	GIVEN( "a UUID" ) {
		EsUuid id;
		THEN( "it always has the same string representation" ) {
			std::string value(id.ToString());
			CHECK( value == id.ToString() );
		}
		THEN( "it compares equal to itself" ) {
			CHECK( id == id );
		}
		AND_GIVEN( "a second UUID" ) {
			EsUuid other;
			THEN( "the two are never equal" ) {
				CHECK( id != other );
				CHECK_FALSE( id == other );
			}
			WHEN( "the second clones the first" ) {
				other.clone(id);
				THEN( "the two are equal" ) {
					CHECK( other == id );
					CHECK_FALSE( other != id );
				}
			}
		}
	}
	GIVEN( "a reorderable collection of UUIDs" ) {
		auto ids = std::vector<EsUuid>{
			EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(),
			EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(), EsUuid(),
		};
		const auto idValues = AsStrings(ids);
		REQUIRE( idValues.size() == ids.size() );
		THEN( "it can be sorted by value" ) {
			std::sort(ids.begin(), ids.end());
			const auto sortedValues = AsStrings(ids);
			CHECK_FALSE( idValues == sortedValues );
			AND_THEN( "it still contains the same UUIDs" ) {
				CHECK( std::is_permutation(idValues.begin(), idValues.end(), sortedValues.begin()) );
			}
		}
	}
}

SCENARIO( "Copying uniquely identifiable objects", "[uuid][copying]" ) {
	// ES generally does not copy identifiable objects, with the sole exception of Ship instances. Copies
	// are currently done when creating ships from a "stock" instance held by GameData, a StartCondition,
	// or when registering a captured NPC. When creating a ship from a stock instance, the source and copy
	// should not share a UUID value. When registering a captured ship, however, the ships should share an
	// identifier.
	// (It is also not required for a ship gifted to a new pilot be strictly identified, just that it
	// can be identified as a starting ship at a later instance. The same goes for ships gifted by missions:
	// a later mission should be able to identify which of the player's ships was gifted by some particular
	// previous mission.)
	GIVEN( "an object owning a UUID" ) {
		Identifiable source;
		std::string sourceId = source.id.ToString();
		WHEN( "a copy is made via constructor" ) {
			Identifiable other(source);
			THEN( "the copy has a different ID" ) {
				CHECK( other.id.ToString() != sourceId );
			}
		}
		WHEN( "a copy is made via assignment" ) {
			Identifiable other = source;
			THEN( "the copy has a different ID" ) {
				CHECK( other.id.ToString() != sourceId );
			}
		}
		WHEN( "a copy is explicitly requested" ) {
			Identifiable other;
			other.id.clone(source.id);
			THEN( "the copy has the same ID string" ) {
				CHECK( other.id.ToString() == sourceId );
			}
			THEN( "the copied ID compares equal to the source" ) {
				CHECK( other.id == source.id );
			}
			THEN( "the copied ID occupies different memory than the source ID" ) {
				CHECK( &other.id != &source.id );
			}
		}
	}
}

SCENARIO( "Mapping identifiable collections", "[uuid][comparison][collections]" ) {
	using T = Identifiable;
	GIVEN( "two objects with the same UUID" ) {
		auto source = std::make_shared<T>();
		auto cloned = std::make_shared<T>();
		cloned->id.clone(source->UUID());
		WHEN( "the collection has a default comparator" ) {
			auto collection = std::set<std::shared_ptr<T>>{};
			REQUIRE( collection.emplace(source).second );
			THEN( "both objects may be added" ) {
				CHECK( collection.emplace(cloned).second );
			}
		}
		WHEN( "the collection uses an ID comparator" ) {
			auto collection = std::set<std::shared_ptr<T>, UUIDComparator<T>>{};
			REQUIRE( collection.emplace(source).second );
			THEN( "only one object may be added" ) {
				CHECK_FALSE( collection.emplace(cloned).second );
			}
		}
	}
	GIVEN( "a collection of items with UUIDs" ) {
		auto collection = std::map<std::shared_ptr<T>, int, UUIDComparator<T>>{};
		auto first = std::make_shared<T>();
		auto second = std::make_shared<T>();
		collection.insert({ {first, -1}, {second, -2} });
		for(int i = 0; i < 10; ++i)
			collection.emplace(std::make_shared<T>(), i);
		THEN( "item retrieval works correctly" ) {
			CHECK( collection.at(first) == -1 );
			CHECK( collection.at(second) == -2 );
		}
	}
}

// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests

// #region benchmarks
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
TEST_CASE( "Benchmark UUID Creation", "[!benchmark][uuid]" ) {
	BENCHMARK( "MakeUuid" ) {
		return es_uuid::detail::MakeUuid();
	};
}
#endif
// #endregion benchmarks



} // test namespace
