# Tests

Endless Sky has different types of automated tests:
- Most "single script checkers" like coding-styles and the parse-test are located under [utils](../utils).
- The unit-tests are located in the [unit](./unit) subdirectory.
- The integration test runners are located in the [integration](./integration) subdirectory.

# Writing New Tests

Choose the best test-technology to get the most maintainable tests and the best test-coverage.

If your test is checking something in the build/test environment, then a single script in the utils directory is likely most fitting.

If your test is really testing the codebase, then a unit-test or an integration test is likely most fitting. When the test-coverage would be equal, then writing a unit-test is preferred over writing an integration test, because unit-tests are generally more efficient and easier to debug.
