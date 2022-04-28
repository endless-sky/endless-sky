# Integration Tests

This directory contains scripts that help the execution of the integration tests.

The actual integration tests are located under [the data directory](../../data/tests)

# Writing Tests

Look at the existing integration tests under the data directory to get started with writing an integration test.

Also consider if the test you want to write can be written as an [unit-test](../unit) instead. Choose the best test-technology to get the best test-coverage. When the test-coverage would be equal, then writing an unit-test is preferred over writing an integration test, because unit-tests are generally more efficient and easier to debug.

## Limitations

Currently (2022-04-26) some features like automatically generated conditions and mouse-inputs are not implemented yet. This means that the framework is limited to giving command and keyboard inputs only and that tests can only check existing conditions already available in the game.
