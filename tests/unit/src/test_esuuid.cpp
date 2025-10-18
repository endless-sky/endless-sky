/* test_esuuid.cpp
Copyright (c) 2021 by Benjamin Hauch

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
#include "logger-output.h"
#include "output-capture.hpp"

// Include only the tested classes' headers.
#include "../../../source/comparators/ByUUID.h"
#include "../../../source/EsUuid.h"

#include "../../../source/Random.h"

// ... and any system includes needed for the test file.
#include <algorithm>
#include <list>
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
		static std::string convert(const EsUuid &value) {
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
struct InstantiableContainer : public Identifiable {
	std::vector<InstantiableContainer> items;
	std::list<InstantiableContainer> others;

	InstantiableContainer() noexcept = default;
	InstantiableContainer(const InstantiableContainer &) noexcept = default;
	InstantiableContainer &operator=(const InstantiableContainer &) noexcept = default;
	InstantiableContainer(InstantiableContainer &&) noexcept = default;
	InstantiableContainer &operator=(InstantiableContainer &&) noexcept = default;

	std::vector<std::string> GetIds() const {
		auto result = std::vector<std::string>{
			id.ToString()
		};
		for(auto &&ic : items)
		{
			auto itemIds = ic.GetIds();
			result.reserve(result.size() + itemIds.size());
			for(std::string &id : itemIds)
				result.push_back(std::move(id));
		}
		for(auto &&ic : others)
		{
			auto itemIds = ic.GetIds();
			result.reserve(result.size() + itemIds.size());
			for(std::string &id : itemIds)
				result.push_back(std::move(id));
		}
		return result;
	}

	InstantiableContainer Instantiate() const {
		InstantiableContainer result;

		for(auto &&ic : items)
			result.items.emplace_back(ic.Instantiate());
		for(auto &&ic : others)
			result.others.emplace_back(ic.Instantiate());

		return result;
	}
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
		CHECK_FALSE( std::is_trivial_v<T> );
		CHECK( std::is_standard_layout_v<T> );
		CHECK( std::is_nothrow_destructible_v<T> );
		CHECK( std::is_trivially_destructible_v<T> );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible_v<T> );
		// Ensuring the memory associated with the UUID is initialized means a non-trivial default constructor.
		CHECK_FALSE( std::is_trivially_default_constructible_v<T> );
		CHECK( std::is_nothrow_default_constructible_v<T> );
		// TODO: enable after refactoring how we create ships from stock models.
		// CHECK_FALSE( std::is_copy_constructible_v<T> );
		CHECK( std::is_move_constructible_v<T> );
		CHECK( std::is_trivially_move_constructible_v<T> );
		CHECK( std::is_nothrow_move_constructible_v<T> );
	}
	// TODO: enable, as above.
	// SECTION( "Copy Traits" ) {
	// 	CHECK_FALSE( std::is_copy_assignable_v<T> );
	// }
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable_v<T> );
		CHECK( std::is_trivially_move_assignable_v<T> );
		CHECK( std::is_nothrow_move_assignable_v<T> );
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
				CHECK( IgnoreLogHeaders(warnings.Flush()) == expected );
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
				other.Clone(id);
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
			other.id.Clone(source.id);
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

SCENARIO( "Constructing uniquely identifiable objects", "[uuid][creation]" ) {
	auto MakeContainer = [](const std::string &parentId, const std::string &childId, const std::string &otherId) {
		InstantiableContainer result;
			result.id = EsUuid::FromString(parentId);
			result.items.emplace_back();
			result.items.front().id = EsUuid::FromString(childId);
			result.others.emplace_back();
			result.others.front().id = EsUuid::FromString(otherId);
			// Add a random number of other items to the containers.
			for(auto i = 0U; i < 1 + Random::Int(4); ++i)
				result.items.emplace_back();
			for(auto i = 0U; i < 1 + Random::Int(4); ++i)
				result.others.emplace_back();
			// Ensure the hierarchy is multiple levels deep.
			result.items.front().others.emplace_back();
			result.others.back().items.emplace_back();
			return result;
	};

	GIVEN( "A template identifiable object" ) {
		const std::string parentId = "cac52c1a-b53d-4edc-92d7-6b2e8ac19434";
		const std::string childItemId = "4d9f7874-4e0c-4904-967b-40b0d20c3e4b";
		const std::string otherId = "ae50c081-ebd2-438a-8655-8a092e34987a";
		const InstantiableContainer source = MakeContainer(parentId, childItemId, otherId);

		WHEN( "the template is instantiated" ) {
			auto instance = source.Instantiate();
			THEN( "all the IDs are new" ) {
				CHECK_FALSE( source.GetIds() == instance.GetIds() );
			}
			AND_WHEN( "the instance is moved" ) {
				const auto preMoveIds = instance.GetIds();
				auto allIds = std::set<std::string>(preMoveIds.begin(), preMoveIds.end());
				const auto initialCount = allIds.size();
				THEN( "the IDs are unchanged" ) {
					auto consumer = std::move(instance);
					const auto postMoveIds = consumer.GetIds();
					allIds.insert(postMoveIds.begin(), postMoveIds.end());
					CHECK( allIds.size() == initialCount );
				}
			}
		}
	}

	GIVEN( "multiple template identifiable objects" ) {
		const auto parentIds = std::vector<std::string>
			{"0ac0837c-bbf8-452a-850d-79d08e667ca7", "33e28130-4e1e-4676-835a-98395c3bc3bb"};
		const auto childIds = std::vector<std::string>
			{"4c5c32ff-bb9d-43b0-b5b4-2d72e54eaaa4", "c4900540-2379-4c75-844b-64e6faf8716b"};
		const auto otherIds = std::vector<std::string>
			{"fd228cb7-ae11-4ae3-864c-16f3910ab8fe", "d9dc8a3b-b784-432e-a781-5a1130a75963"};

		std::map<unsigned, InstantiableContainer> collection;
		collection.emplace(0U, MakeContainer(parentIds[0], childIds[0], otherIds[0]));
		collection.emplace(1U, MakeContainer(parentIds[1], childIds[1], otherIds[1]));
		auto allIds = std::set<std::string>{};
		for(auto &&it : collection)
		{
			auto ids = it.second.GetIds();
			allIds.insert(ids.begin(), ids.end());
		}
		for(auto &&list : {parentIds, childIds, otherIds})
			for(auto &&id : list)
			{
				UNSCOPED_INFO( "Collection IDs should include seed ID " + id );
				REQUIRE( allIds.count(id) == 1 );
			}

		WHEN( "all templates are instantiated" ) {
			auto results = std::list<InstantiableContainer>{};
			for(auto &&it : collection)
				results.emplace_back(it.second.Instantiate());

			THEN( "all IDs are unique" ) {
				unsigned num = 1;
				for(auto &&ic : results)
				{
					auto instanceIds = ic.GetIds();
					for(auto &&id : instanceIds)
					{
						UNSCOPED_INFO( "added id " + std::to_string(num) + " is " + id );
						CHECK( allIds.insert(id).second );
						++num;
					}
				}
			}
		}
	}
}


SCENARIO( "Mapping identifiable collections", "[uuid][comparison][collections]" ) {
	using T = Identifiable;
	GIVEN( "two objects with the same UUID" ) {
		auto source = std::make_shared<T>();
		auto cloned = std::make_shared<T>();
		cloned->id.Clone(source->UUID());
		WHEN( "the collection has a default comparator" ) {
			auto collection = std::set<std::shared_ptr<T>>{};
			REQUIRE( collection.emplace(source).second );
			THEN( "both objects may be added" ) {
				CHECK( collection.emplace(cloned).second );
			}
		}
		WHEN( "the collection uses an ID comparator" ) {
			auto collection = std::set<std::shared_ptr<T>, ByUUID<T>>{};
			REQUIRE( collection.emplace(source).second );
			THEN( "only one object may be added" ) {
				CHECK_FALSE( collection.emplace(cloned).second );
			}
		}
	}
	GIVEN( "a collection of items with UUIDs" ) {
		auto collection = std::map<std::shared_ptr<T>, int, ByUUID<T>>{};
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
	GIVEN( "a collection of strings as ID comparator, with UUIDs, identifying items" ) {
		auto collection = std::map<std::string, EsUuid>{};
		Identifiable first;
		Identifiable second;
		std::string firstName = "one";
		std::string secondName = "two";
		collection.insert({ {firstName, EsUuid()}, {secondName, EsUuid()} });
		WHEN( "we use strings to find the corresponding UUID in the collection" ) {
			collection.at(firstName).Clone(first.id);
			collection.at(secondName).Clone(second.id);
			THEN( "we can use them to identify the items in a unique way" ) {
				CHECK( collection.at(firstName) == first.id );
				CHECK( collection.at(secondName) == second.id );
			}
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
		return EsUuid::MakeUuid();
	};
}
#endif
// #endregion benchmarks



} // test namespace
