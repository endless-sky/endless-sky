# Integration Tests

This directory contains scripts that help the execution of the integration tests.

The actual integration tests are located under [the data directory](config/plugins/integration-tests/data/tests/) and in the future there will also be test-universes with embedded integration tests under the current directory (for testing features that are in the codebase, but not in the datafiles for the main game).

# Writing Integration Tests

Look at the existing integration tests under the data directory to get started with writing an integration test.

## Limitations

Currently (2022-04-26) some features like automatically generated conditions and mouse-inputs are not implemented yet. This means that the framework is limited to giving command and keyboard inputs only and that tests can only check existing conditions already available in the game.
