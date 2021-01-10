#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/Cache.h"

// ... and any system includes needed for the test file.
#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

namespace { // test namespace
// #region mock data
using Key = int;
using T = std::string;

std::vector<T> recycleRecord = {};
class AtRecycle {
public:
	void operator()(T &data) const
	{
		recycleRecord.push_back(data);
	}
};

bool Exist(const std::vector<T> &vector, const T &value)
{
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}
// #endregion mock data


// #region unit tests
TEST_CASE( "Cache class", "[Cache]" ) {
	SECTION( "Class Traits" ) {
		CHECK( std::is_default_constructible<Cache<Key, T>>::value );
		CHECK( std::is_move_constructible<Cache<Key, T>>::value );
		CHECK( std::is_move_assignable<Cache<Key, T>>::value );
		CHECK( std::is_nothrow_move_assignable<Cache<Key, T>>::value );
		CHECK( std::is_destructible<Cache<Key, T>>::value );
		CHECK( std::is_nothrow_destructible<Cache<Key, T>>::value );
		CHECK( std::has_virtual_destructor<Cache<Key, T>>::value );
	}
	SECTION( "Return Value" ) {
		Cache<Key, T> cache;
		CHECK( cache.Set(1, "a") == "a" );
		CHECK( cache.New(2, "b") == "b" );
		const auto u = cache.Use(2);
		REQUIRE( u.second );
		CHECK( *u.first == "b" );
	}
	SECTION( "Recycle timing" ) {
		Cache<Key, T, true> cache;
		cache.SetUpdateInterval(1);
		cache.Set(1, "a");
		CHECK_FALSE( cache.Recycle().second );
		CacheBase::Step();
		CHECK_FALSE( cache.Recycle().second );
		CacheBase::Step();
		CHECK( cache.Recycle().second );
	}
}

SCENARIO( "Life cycle of the objects in a cache", "[Cache]" ) {
	GIVEN( "manual expired cache that didn't be called Expire()" ) {
		recycleRecord.clear();
		Cache<Key, T, false, std::hash<Key>, AtRecycle> cache;
		cache.SetUpdateInterval(1);
		cache.Set(1, "a");
		cache.Set(2, "b");
		cache.Set(3, "c");
		CacheBase::Step();
		cache.Set(4, "d");
		cache.Set(5, "e");
		CacheBase::Step();
		cache.Set(6, "f");
		
		WHEN( "Recyle() is called" ) {
			THEN( "there is no expired entry." ) {
				CHECK_FALSE( cache.Recycle().second );
			}
		}
		WHEN( "Use() is called" ) {
			THEN( "all entries can use." ) {
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
				CHECK( cache.Use(5).second );
				CHECK( cache.Use(6).second );
			}
		}
		WHEN( "Clear() is called" ) {
			cache.Clear();
			THEN( "all entries are disappeared." ) {
				CHECK_FALSE( cache.Use(1).second );
				CHECK_FALSE( cache.Use(2).second );
				CHECK_FALSE( cache.Use(3).second );
				CHECK_FALSE( cache.Use(4).second );
				CHECK_FALSE( cache.Use(5).second );
				CHECK_FALSE( cache.Use(6).second );
				REQUIRE( recycleRecord.size() == 6 );
				CHECK( Exist(recycleRecord, "a") );
				CHECK( Exist(recycleRecord, "b") );
				CHECK( Exist(recycleRecord, "c") );
				CHECK( Exist(recycleRecord, "d") );
				CHECK( Exist(recycleRecord, "e") );
				CHECK( Exist(recycleRecord, "f") );
			}
		}
	}
	GIVEN( "manual expired cache that was called Expire()" ) {
		recycleRecord.clear();
		Cache<Key, T, false, std::hash<Key>, AtRecycle> cache;
		cache.SetUpdateInterval(1);
		cache.Set(1, "a");
		cache.Set(2, "b");
		cache.Set(3, "c");
		cache.Expire(1);
		CacheBase::Step();
		
		WHEN( "Recyle() is called" ) {
			const auto a = cache.Recycle();
			THEN( "there is an expired entry." ) {
				CHECK( a.first == "a" );
				CHECK( a.second );
				CHECK( recycleRecord.size() == 0 );
				CHECK_FALSE( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
			}
		}
		WHEN( "Use() is called" ) {
			THEN( "all entries can use." ) {
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
			}
		}
		WHEN( "Set() is called" ) {
			cache.Set(4, "d");
			THEN( "The expired entry is disappeared." ) {
				REQUIRE( recycleRecord.size() == 1 );
				CHECK( recycleRecord[0] == "a" );
				CHECK_FALSE( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
			}
		}
		WHEN( "New() is called" ) {
			cache.New(4, "d");
			THEN( "all entries can use." ) {
				CHECK( recycleRecord.size() == 0 );
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
			}
		}
		WHEN( "Clear() is called" ) {
			cache.Clear();
			THEN( "all entries are disappeared." ) {
				CHECK_FALSE( cache.Use(1).second );
				CHECK_FALSE( cache.Use(2).second );
				CHECK_FALSE( cache.Use(3).second );
				REQUIRE( recycleRecord.size() == 3 );
				CHECK( Exist(recycleRecord, "a") );
				CHECK( Exist(recycleRecord, "b") );
				CHECK( Exist(recycleRecord, "c") );
			}
		}
	}
	GIVEN( "auto expired cache" ) {
		recycleRecord.clear();
		Cache<Key, T, true, std::hash<Key>, AtRecycle> cache;
		cache.SetUpdateInterval(1);
		cache.Set(1, "a");
		cache.Set(2, "b");
		cache.Set(3, "c");
		CacheBase::Step();
		cache.Set(4, "d");
		cache.Set(5, "e");
		CacheBase::Step();
		
		WHEN( "Recyle() is called" ) {
			THEN( "there three no expired entry." ) {
				CHECK( cache.Recycle().second );
				CHECK( cache.Recycle().second );
				CHECK( cache.Recycle().second );
				CHECK_FALSE( cache.Recycle().second );
			}
		}
		WHEN( "Use() is called" ) {
			THEN( "all entries can use." ) {
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
				CHECK( cache.Use(5).second );
			}
		}
		WHEN( "Set() is called" ) {
			cache.Set(6, "f");
			THEN( "The expired entry is disappeared." ) {
				REQUIRE( recycleRecord.size() == 1 );
				const bool a_or_b_or_c = recycleRecord[0] == "a" || recycleRecord[0] == "b" || recycleRecord[0] == "c";
				CHECK( a_or_b_or_c );
				const int number_of_valid_entry = (cache.Use(1).second ? 1 : 0) + (cache.Use(2).second ? 1 : 0)
					+ (cache.Use(3).second ? 1 : 0);
				CHECK( number_of_valid_entry == 2 );
				CHECK( cache.Use(4).second );
				CHECK( cache.Use(5).second );
				CHECK( cache.Use(6).second );
			}
		}
		WHEN( "New() is called" ) {
			cache.New(6, "f");
			THEN( "all entries can use." ) {
				CHECK( recycleRecord.size() == 0 );
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
				CHECK( cache.Use(5).second );
				CHECK( cache.Use(6).second );
			}
		}
		WHEN( "Clear() is called" ) {
			cache.Clear();
			THEN( "all entries are disappeared." ) {
				CHECK_FALSE( cache.Use(1).second );
				CHECK_FALSE( cache.Use(2).second );
				CHECK_FALSE( cache.Use(3).second );
				CHECK_FALSE( cache.Use(4).second );
				CHECK_FALSE( cache.Use(5).second );
				REQUIRE( recycleRecord.size() == 5 );
				CHECK( Exist(recycleRecord, "a") );
				CHECK( Exist(recycleRecord, "b") );
				CHECK( Exist(recycleRecord, "c") );
				CHECK( Exist(recycleRecord, "d") );
				CHECK( Exist(recycleRecord, "e") );
			}
		}
	}
	GIVEN( "manual expired cache that was called Use() twice" ) {
		recycleRecord.clear();
		Cache<Key, T, false, std::hash<Key>, AtRecycle> cache;
		cache.SetUpdateInterval(1);
		cache.Set(1, "a");
		cache.Set(2, "b");
		cache.Set(3, "c");
		cache.Use(1);
		cache.Use(1);
		
		WHEN( "Expire() and CacheBase::Step() are called" ) {
			cache.Expire(1);
			CacheBase::Step();
			THEN( "there is no expired entry." ) {
				CHECK_FALSE( cache.Recycle().second );
				CHECK( recycleRecord.size() == 0 );
			}
		}
		WHEN( "2x Expire() and CacheBase::Step() are called" ) {
			cache.Expire(1);
			cache.Expire(1);
			CacheBase::Step();
			THEN( "there is no expired entry." ) {
				CHECK_FALSE( cache.Recycle().second );
				CHECK( recycleRecord.size() == 0 );
			}
		}
		WHEN( "3x Expire() and CacheBase::Step() are called" ) {
			cache.Expire(1);
			cache.Expire(1);
			cache.Expire(1);
			CacheBase::Step();
			THEN( "there is an expired entry." ) {
				CHECK( cache.Recycle().second );
				CHECK( recycleRecord.size() == 0 );
			}
		}
	}
}
SCENARIO( "Moved objects", "[Cache]" ) {
	GIVEN( "A cache moved from an empty cache" ) {
		recycleRecord.clear();
		Cache<Key, T, false, std::hash<Key>, AtRecycle> cache2;
		cache2.SetUpdateInterval(1);
		auto cache = std::move(cache2);
		
		WHEN( "Set() is called" ) {
			cache.Set(1, "a");
			THEN( "there is an entry." ) {
				CHECK( cache.Use(1).second );
			}
		}
		WHEN( "Set() and Expire() are called" ) {
			cache.Set(1, "a");
			cache.Expire(1);
			THEN( "there is one expired entry." ) {
				CacheBase::Step();
				CHECK( cache.Recycle().second );
			}
		}
	}
	GIVEN( "A cache moved from a cache that has only expired entry" ) {
		recycleRecord.clear();
		Cache<Key, T, true, std::hash<Key>, AtRecycle> cache2;
		cache2.SetUpdateInterval(1);
		cache2.Set(1, "a");
		CacheBase::Step();
		auto cache = std::move(cache2);
		
		WHEN( "Set() is called" ) {
			cache.Set(2, "b");
			THEN( "there is two entry." ) {
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
			}
		}
		WHEN( "CacheBase;:Step() and Set() are called" ) {
			CacheBase::Step();
			cache.Set(2, "b");
			THEN( "there is an entry." ) {
				CHECK_FALSE( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				REQUIRE( recycleRecord.size() == 1 );
				CHECK( Exist(recycleRecord, "a") );
			}
		}
	}
	GIVEN( "A cache moved from a cache that has only readyToRecycle entry" ) {
		recycleRecord.clear();
		Cache<Key, T, true, std::hash<Key>, AtRecycle> cache2;
		cache2.SetUpdateInterval(1);
		cache2.Set(1, "a");
		CacheBase::Step();
		CacheBase::Step();
		auto cache = std::move(cache2);
		
		WHEN( "Set() is called" ) {
			cache.Set(2, "b");
			THEN( "there is an entry." ) {
				CHECK_FALSE( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				REQUIRE( recycleRecord.size() == 1 );
				CHECK( Exist(recycleRecord, "a") );
			}
		}
	}
	GIVEN( "manual expired cache that has various entries" ) {
		recycleRecord.clear();
		Cache<Key, T, false, std::hash<Key>, AtRecycle> cache2;
		cache2.SetUpdateInterval(1);
		cache2.Set(1, "a");
		cache2.Set(2, "b");
		cache2.Set(3, "c");
		cache2.Expire(1);
		CacheBase::Step();
		cache2.New(4, "d");
		cache2.New(5, "e");
		CacheBase::Step();
		cache2.New(6, "f");
		auto cache = std::move(cache2);
		
		WHEN( "Recyle() is called" ) {
			const auto a = cache.Recycle();
			THEN( "there is an expired entry." ) {
				CHECK( a.first == "a" );
				CHECK( a.second );
				CHECK( recycleRecord.size() == 0 );
				CHECK_FALSE( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
			}
		}
		WHEN( "Use() is called" ) {
			THEN( "all entries can use." ) {
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
				CHECK( cache.Use(5).second );
				CHECK( cache.Use(6).second );
			}
		}
		WHEN( "Set() is called" ) {
			cache.Set(7, "g");
			THEN( "The expired entry is disappeared." ) {
				REQUIRE( recycleRecord.size() == 1 );
				CHECK( recycleRecord[0] == "a" );
				CHECK_FALSE( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
				CHECK( cache.Use(5).second );
				CHECK( cache.Use(6).second );
				CHECK( cache.Use(7).second );
			}
		}
		WHEN( "New() is called" ) {
			cache.New(7, "g");
			THEN( "all entries can use." ) {
				CHECK( recycleRecord.size() == 0 );
				CHECK( cache.Use(1).second );
				CHECK( cache.Use(2).second );
				CHECK( cache.Use(3).second );
				CHECK( cache.Use(4).second );
				CHECK( cache.Use(5).second );
				CHECK( cache.Use(6).second );
				CHECK( cache.Use(7).second );
			}
		}
		WHEN( "Clear() is called" ) {
			cache.Clear();
			THEN( "all entries are disappeared." ) {
				CHECK_FALSE( cache.Use(1).second );
				CHECK_FALSE( cache.Use(2).second );
				CHECK_FALSE( cache.Use(3).second );
				CHECK_FALSE( cache.Use(4).second );
				CHECK_FALSE( cache.Use(5).second );
				CHECK_FALSE( cache.Use(6).second );
				REQUIRE( recycleRecord.size() == 6 );
				CHECK( Exist(recycleRecord, "a") );
				CHECK( Exist(recycleRecord, "b") );
				CHECK( Exist(recycleRecord, "c") );
				CHECK( Exist(recycleRecord, "d") );
				CHECK( Exist(recycleRecord, "e") );
				CHECK( Exist(recycleRecord, "f") );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
