# Unit Tests

This directory contains files that link against a particular source code file used by the game. All test files are required to be prefixed with `test_`. Any C++ source files not prefixed with `test_` will be ignored by the build system.

Tests should be added in their own file, named "test_classToBeTested.cpp"  
If the test file will become cumbersome, a subdirectory structure is OK:

```
tests/
  ...
  src/ (this directory)
    test_classA.cpp
    ClassToBeTested/
      test_classToBeTested-methodToBeTested.cpp
        ...
    ...
```

# Writing Tests

To get started, copy the `test_template.txt` file to a new source file, e.g. `cp test_template.txt test_foo.cpp`.

1. We do not want tests, variables, classes, fixtures, etc. that are needed to test the class or method to be accessible to other files. Thus, all test code is to be wrapped in an anonymous namespace.
2. Prefer scenario-driven language that uses the `SCENARIO`, `GIVEN`, `WHEN`, and `THEN` macros over the test-driven language of `TEST_CASE` and `SECTION` macros. (There will be situations where the latter is better-suited to the class / method under test.)
3. When writing assertions, prefer the `CHECK` and `CHECK_FALSE` macros when probing the scenario, and prefer the `REQUIRE` / `REQUIRE_FALSE` macros for fundamental / "validity" assertions. If a `CHECK` fails, the rest of the block's statements will still be evaluated, but a `REQUIRE` failure will exit the current block.

## Limitations

Often we cannot test a class using just the class and primitives such as `int`s or `double`s - we need classes, classes that expose a particular interface, or even calls to third party libraries like SDL. For unit tests, we do _**NOT**_ want to actually make these calls to other implementations, so we declare file-local mocks and/or stubs that allow us to inspect or control these calls, for the sake of testing our particular implementation.

Unfortunately, the current architecture & implementation of Endless Sky's code does not support dependency injection, with which we could provide mocked implementations to the actual game code. Such capability would enable assertions of the behavior of the code under test when other code units behave in prescribed manners. Effectively, we are not able to write unit tests for code that is coupled with external dependencies (i.e. much of the code we would like to test). We can, however, still write unit tests for classes whose behavior is not driven by external dependencies, and ensure that their behavior is sound.

The game and the test binary are assembled using link-time optimization (LTO, "whole-program optimization"), which means we are able to test _some_ aspects of classes that use 3rd party dependencies, so long as the tested methods do not actually use or depend upon code that uses the dependencies. Generally speaking, if a method invokes the `GameData` class, it will not be testable using unit tests.
