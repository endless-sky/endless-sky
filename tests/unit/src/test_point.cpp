/* test_point.cpp
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
#include "../../../source/Point.h"

// ... and any system includes needed for the test file.
#include <cmath>
#include <limits>



namespace { // test namespace
// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "Point Basics", "[Point]" ) {
	using T = Point;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial_v<T> );
		CHECK( std::is_standard_layout_v<T> );
		CHECK( std::is_nothrow_destructible_v<T> );
		CHECK( std::is_trivially_destructible_v<T> );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible_v<T> );
		CHECK_FALSE( std::is_trivially_default_constructible_v<T> );
		CHECK( std::is_nothrow_default_constructible_v<T> );
		CHECK( std::is_copy_constructible_v<T> );
		CHECK( std::is_trivially_copy_constructible_v<T> );
		CHECK( std::is_nothrow_copy_constructible_v<T> );
		CHECK( std::is_move_constructible_v<T> );
		CHECK( std::is_trivially_move_constructible_v<T> );
		CHECK( std::is_nothrow_move_constructible_v<T> );
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable_v<T> );
		CHECK( std::is_trivially_copyable_v<T> );
		CHECK( std::is_trivially_copy_assignable_v<T> );
		CHECK( std::is_nothrow_copy_assignable_v<T> );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable_v<T> );
		CHECK( std::is_trivially_move_assignable_v<T> );
		CHECK( std::is_nothrow_move_assignable_v<T> );
	}
}

SCENARIO( "A position or other geometric vector must be expressed", "[Point]" ) {
	GIVEN( "No initial values" ) {
		Point a;
		WHEN( "the point is created" ) {
			THEN( "it represents (0, 0)" ) {
				CHECK( a.X() == 0. );
				CHECK( a.Y() == 0. );
			}
		}
		WHEN( "Set is called" ) {
			a.Set(1, 3);
			THEN( "X and Y are updated" ) {
				CHECK( a.X() == 1. );
				CHECK( a.Y() == 3. );
			}
		}
	}

	GIVEN( "any Point" ) {
		auto a = Point();
		WHEN( "the point represents (0, 0)" ) {
			a.X() = 0.;
			a.Y() = 0.;
			THEN( "it can be converted to boolean FALSE" ) {
				CHECK( static_cast<bool>(a) == false );
			}

			THEN( "it is equal to the default point" ) {
				CHECK( a == Point() );
			}
		}
		WHEN( "the point has non-zero X" ) {
			a.X() = 0.00001;
			REQUIRE( a.Y() == 0. );
			THEN( "it can be converted to boolean TRUE" ) {
				CHECK( static_cast<bool>(a) == true );
			}

			THEN( "it isn't equal to the default point" ) {
				CHECK( a != Point() );
			}
		}
		WHEN( "the point has non-zero Y") {
			a.Y() = 0.00001;
			REQUIRE( a.X() == 0. );
			THEN( "it can be converted to boolean TRUE" ) {
				CHECK( static_cast<bool>(a) == true );
			}
		}
	}

	GIVEN( "two points" ) {
		auto a = Point();
		auto b = Point();

		WHEN( "both points are (0, 0)" ) {
			THEN( "they are equal" ) {
				CHECK( a == b );
			}
		}

		WHEN( "one point is different" ) {
			a.X() = 0.0001;
			THEN( "they are not equal" ) {
				CHECK( a != b );
			}
		}
	}
}

SCENARIO( "Copying Points", "[Point]" ) {
	GIVEN( "any Point" ) {
		auto source = Point(5.4321, 10.987654321);
		WHEN( "copied by constructor" ) {
			Point copy(source);
			THEN( "the copy has the correct values" ) {
				CHECK( copy.X() == source.X() );
				CHECK( copy.Y() == source.Y() );
			}
		}
		WHEN( "copied by assignment" ) {
			Point copy = source;
			THEN( "the copy has the correct values" ) {
				CHECK( copy.X() == source.X() );
				CHECK( copy.Y() == source.Y() );
			}
		}
	}
}

SCENARIO( "Adding points", "[Point][operator+]" ) {
	GIVEN( "any Point" ) {
		Point first = Point(5.4321, 10.987654321);
		WHEN( "Nothing is added" ) {
			Point second;
			Point third;
			THEN( "The sum is the same as the original" ) {
				CHECK( first == (first + second) );
				CHECK( first == (second + first) );
				third = first;
				third += second;
				CHECK( first == third );
				third = second;
				third += first;
				CHECK( first == third );
			}
		}
		WHEN( "A value is added" ) {
			Point second = Point(25.4321, 10.487254321);
			Point third;
			Point expected = Point(first.X() + second.X(), first.Y() + second.Y());
			THEN( "The sum is has the expected value" ) {
				CHECK( expected == (first + second) );
				CHECK( expected == (second + first) );
				third = first;
				third += second;
				CHECK( expected == third );
				third = second;
				third += first;
				CHECK( expected == third );
			}
		}
	}
}

SCENARIO( "Subtracting points", "[Point][operator-]" ) {
	GIVEN( "any Point" ) {
		Point first = Point(5.4321, 10.987654321);
		WHEN( "Nothing is subtracted" ) {
			Point second;
			Point third;
			THEN( "The result is the same as the original" ) {
				CHECK( first == (first - second) );
				CHECK( -first == (second - first) );
				third = first;
				third -= second;
				CHECK( first == third );
				third = second;
				third -= first;
				CHECK( -first == third );
			}
		}
		WHEN( "A value is subtracted" ) {
			Point second = Point(25.4321, 10.487254321);
			Point third;
			Point expected = Point(first.X() - second.X(), first.Y() - second.Y());
			THEN( "The sum is has the expected value" ) {
				CHECK( expected == (first - second) );
				CHECK( -expected == (second - first) );
				third = first;
				third -= second;
				CHECK( expected == third );
				third = second;
				third -= first;
				CHECK( -expected == third );
			}
		}
	}
}

SCENARIO( "Multiplying points", "[Point][operator*]" ) {
	GIVEN( "any Point" ) {
		Point first = Point(5.4321, 10.987654321);
		WHEN( "Multiplied with 1" ) {
			THEN( "The result is the same as the original" ) {
				CHECK( first == (first * 1.) );
				CHECK( first == (1. * first) );
			}
		}
		WHEN( "Multiplied with 0" ) {
			THEN( "The result is 0" ) {
				CHECK( Point() == (first * 0.) );
				CHECK( Point() == (0. * first) );
			}
		}
		WHEN( "Multiplied with a number" )
		{
			double mult = 25.25406;
			Point expected(first.X() * mult, first.Y() * mult);
			THEN( "The result is correct" ) {
				CHECK( expected == first * mult );
				CHECK( expected == mult * first );
			}
		}
	}
}

SCENARIO( "Dividing points", "[Point][operator/]" ) {
	GIVEN( "any Point" ) {
		Point first = Point(5.4321, 10.987654321);
		WHEN( "Divided by 1" ) {
			THEN( "The result is the same as the original" ) {
				CHECK( first == (first / 1.) );
			}
		}
		WHEN( "Divided by 0" ) {
			THEN( "The result is infinity" ) {
				CHECK( std::numeric_limits<double>::infinity() == (first / 0.).X() );
				CHECK( std::numeric_limits<double>::infinity() == (first / 0.).Y() );
			}
		}
		WHEN( "Divided by a number" )
		{
			double div = 25.25406;
			Point expected(first.X() / div, first.Y() / div);
			THEN( "The result is correct" ) {
				CHECK( expected == first / div );
			}
		}
	}
}

SCENARIO( "Multiplying points with each other", "[Point][operator*]" ) {
	GIVEN( "any two Points" ) {
		Point first = Point(5.4321, 10.987654321);
		Point second = Point(63.57151, 0.156123);
		WHEN( "Multiplied with each other" ) {
			Point expected(first.X() * second.X(), first.Y() * second.Y());
			THEN( "The result is correct" ) {
				CHECK( expected == first * second );
				CHECK( expected == second * first );
			}
		}
	}
}

SCENARIO( "Calculating dot product", "[Point][Dot]" ) {
	GIVEN( "any two Points" ) {
		Point first = Point(5.4321, 10.987654321);
		Point second = Point(63.57151, 0.156123);
		WHEN( "Multiplied with each other" ) {
			double expected = first.X() * second.X() + first.Y() * second.Y();
			THEN( "The result is correct" ) {
				CHECK( expected == first.Dot(second) );
				CHECK( expected == second.Dot(first) );
			}
		}
	}
}

SCENARIO( "Calculating cross product", "[Point][Dot]" ) {
	GIVEN( "any two Points" ) {
		Point first = Point(5.4321, 10.987654321);
		Point second = Point(63.57151, 0.156123);
		WHEN( "Multiplied with each other" ) {
			double expected = first.X() * second.Y() - first.Y() * second.X();
			THEN( "The result is correct" ) {
				CHECK( expected == first.Cross(second) );
				CHECK( -expected == second.Cross(first) );
			}
		}
	}
}

SCENARIO( "Calculating length", "[Point][Length]" ) {
	GIVEN( "any Point" ) {
		Point first = Point(5.4321, 10.987654321);
		WHEN( "Checking length" ) {
			double expectedSq = first.X() * first.X() + first.Y() * first.Y();
			double expected = sqrt(expectedSq);
			THEN( "The result is correct" ) {
				CHECK_THAT( first.Length(), Catch::Matchers::WithinAbs(expected, 0.0001) );
				CHECK_THAT( first.LengthSquared(), Catch::Matchers::WithinAbs(expectedSq, 0.0001) );
			}
		}
	}
}

SCENARIO( "Calculating unit vector", "[Point][Unit]" ) {
	GIVEN( "any Point" ) {
		Point first = Point(5.4321, 10.987654321);
		WHEN( "Calculating unit" ) {
			Point expected = first / first.Length();
			THEN( "The result is correct" ) {
				CHECK_THAT( first.Unit().X(), Catch::Matchers::WithinAbs(expected.X(), 0.0001) );
				CHECK_THAT( first.Unit().Y(), Catch::Matchers::WithinAbs(expected.Y(), 0.0001) );
			}
		}
	}
}

SCENARIO( "Calculating distance", "[Point][Distance]" ) {
	GIVEN( "any two Points" ) {
		Point first = Point(5.4321, 10.987654321);
		Point second = Point(63.57151, 0.156123);
		WHEN( "Calculating distance" ) {
			Point delta = first - second;
			double expected = delta.Length();
			double expectedSq = delta.LengthSquared();
			THEN( "The result is correct" ) {
				CHECK_THAT( first.Distance(second), Catch::Matchers::WithinAbs(expected, 0.0001) );
				CHECK_THAT( second.Distance(first), Catch::Matchers::WithinAbs(expected, 0.0001) );
				CHECK_THAT( first.DistanceSquared(second), Catch::Matchers::WithinAbs(expectedSq, 0.0001) );
				CHECK_THAT( second.DistanceSquared(first), Catch::Matchers::WithinAbs(expectedSq, 0.0001) );
			}
		}
	}
}

SCENARIO( "Linear interpolation", "[Point][Lerp]" ) {
	GIVEN( "any two Points" ) {
		Point first = Point(5.4321, 10.987654321);
		Point second = Point(63.57151, 0.156123);
		WHEN( "Interpolating the first position" ) {
			THEN( "The result is the first point" ) {
				CHECK_THAT( first.Distance(first.Lerp(second, 0.)), Catch::Matchers::WithinAbs(0., 0.0001) );
			}
		}
		WHEN( "Interpolating the second position" ) {
			THEN( "The result is the second point" ) {
				CHECK_THAT( second.Distance(first.Lerp(second, 1.)), Catch::Matchers::WithinAbs(0., 0.0001) );
			}
		}
		WHEN( "Interpolating between them" ) {
			double c = 0.2637;
			Point delta = second - first;
			Point offset = delta * c;
			Point expected = first + offset;
			THEN( "The result is correct" ) {
				Point result = first.Lerp(second, c);
				CHECK_THAT( expected.Distance(result), Catch::Matchers::WithinAbs(0., 0.0001) );
			}
			THEN( "The inverse result is correct" ) {
				Point result = second.Lerp(first, 1. - c);
				CHECK_THAT( expected.Distance(result), Catch::Matchers::WithinAbs(0., 0.0001) );
			}
		}
	}
}

SCENARIO( "Calculating absolute value", "[Point][abs]" ) {
	GIVEN( "any Point" ) {
		Point first = Point(5.4321, -10.987654321);
		WHEN( "Calculating abs" ) {
			THEN( "The result is correct" ) {
				CHECK( std::abs(first.X()) == abs(first).X() );
				CHECK( std::abs(first.Y()) == abs(first).Y() );
			}
		}
	}
}

SCENARIO( "Calculating min/max", "[Point][min]" ) {
	GIVEN( "any two Points" ) {
		Point first = Point(5.4321, 10.987654321);
		Point second = Point(-63.57151, 0.156123);
		WHEN( "Calculating min" ) {
			THEN( "The result is the minimum" ) {
				CHECK( std::min(first.X(), second.X()) == min(first, second).X() );
				CHECK( std::min(first.Y(), second.Y()) == min(first, second).Y() );
			}
		}
		WHEN( "Calculating max" ) {
			THEN( "The result is the maximum" ) {
				CHECK( std::max(first.X(), second.X()) == max(first, second).X() );
				CHECK( std::max(first.Y(), second.Y()) == max(first, second).Y() );
			}
		}
	}
}
// #endregion unit tests

// #region benchmarks
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
TEST_CASE( "Benchmark Point::Point()", "[!benchmark][point][point]" ) {
	BENCHMARK( "Point::Point()" ) {
		return Point();
	};
	BENCHMARK( "Point::Point(x, y)" ) {
		return Point(31261.0501, 1.16026123);
	};

	Point toCopy(31261.0501, 1.16026123);
	BENCHMARK( "Point::Point(Point)" ) {
		return Point(toCopy);
	};
}

TEST_CASE( "Benchmark Point-boolean conversions", "[!benchmark][point][bool]" ) {
	Point zero;
	BENCHMARK( "Zero point" ) {
		return zero.operator bool();
	};
	BENCHMARK( "Negate zero point" ) {
		return !zero;
	};
	Point point(31261.0501, 1.16026123);
	BENCHMARK( "Nonzero point" ) {
		return point.operator bool();
	};
	BENCHMARK( "Negate nonzero point" ) {
		return !point;
	};
}

TEST_CASE( "Benchmark Point arithmetics", "[!benchmark][point][arithmetics]" ) {
	Point first = Point(5.4321, 10.987654321);
	Point second = Point(-63.57151, 0.156123);
	BENCHMARK( "Addition" ) {
		return first + second;
	};
	BENCHMARK( "Subtraction" ) {
		return first - second;
	};
	BENCHMARK( "Lane-wise multiplication" ) {
		return first * second;
	};
	double op = 36.61376183;
	BENCHMARK( "Scalar multiplication" ) {
		return first * op;
	};
	BENCHMARK( "Scalar division" ) {
		return first / op;
	};
}

TEST_CASE( "Benchmark Point vector arithmetics", "[!benchmark][point][vector arithmetics]" ) {
	Point first = Point(5.4321, 10.987654321);
	Point second = Point(-63.57151, 0.156123);
	BENCHMARK( "Dot product" ) {
		return first.Dot(second);
	};
	BENCHMARK( "Cross product" ) {
		return first.Cross(second);
	};
	BENCHMARK( "Length" ) {
		return first.Length();
	};
	BENCHMARK( "LengthSquared" ) {
		return first.LengthSquared();
	};
	BENCHMARK( "Unit" ) {
		return first.Unit();
	};
	BENCHMARK( "Distance" ) {
		return first.Distance(second);
	};
	BENCHMARK( "DistanceSquared" ) {
		return first.DistanceSquared(second);
	};
	BENCHMARK( "Lerp" ) {
		return first.Lerp(second, 0.3167116);
	};
}

TEST_CASE( "Benchmark Point helpers", "[!benchmark][point][helpers]" )
{
	Point first = Point(5.4321, 10.987654321);
	Point second = Point(-63.57151, 0.156123);
	BENCHMARK( "abs" ) {
		return abs(first);
	};
	BENCHMARK( "min" ) {
		return min(first, second);
	};
	BENCHMARK( "max" ) {
		return max(first, second);
	};
}
#endif
// #endregion benchmarks


} // test namespace
