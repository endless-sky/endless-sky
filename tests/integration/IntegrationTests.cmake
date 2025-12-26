set(ES_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/integration/config")

# Get all the tests to run.
execute_process(
	COMMAND ${ES} --config "${ES_CONFIG}" --tests
	OUTPUT_VARIABLE INTEGRATION_TESTS
	ERROR_QUIET
)
# Delete the errors.txt file if any. This file is generated if there were
# parse errors, but we don't care about those.
file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/integration/config/errors.txt")

string(REPLACE "\n" ";" INTEGRATION_TESTS_LIST "${INTEGRATION_TESTS}")
set(TEST_CONFIGS "${BINARY_PATH}/integration_configs")

# Add each test as CTest.
foreach(test ${INTEGRATION_TESTS_LIST})
	# Launches the integration tests in release mode: In the background and as fast as possible.
	set(ADD_TEST
	"add_test([==[${test}]==] \"${CMAKE_COMMAND}\"
		\"-DES=${ES}\"
		\"-DTEST_CONFIGS=${TEST_CONFIGS}\"
		\"-Dtest=${test}\"
		\"-DRESOURCE_PATH=${RESOURCE_PATH}\"
		\"-DES_CONFIG=${ES_CONFIG}\"
		-P \"${CMAKE_SOURCE_DIR}/integration/RunIntegrationTest.cmake\")")
		set(SET_TEST_PROPS
	"set_tests_properties([==[${test}]==] PROPERTIES
		WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"
		TIMEOUT 120
		LABELS integration)")

	# Launches the integration tests in debug mode, so that they can be followed.
	set(ADD_TEST_DEBUG
	"add_test([==[[debug] ${test}]==] \"${CMAKE_COMMAND}\"
		\"-DES=${ES}\"
		\"-DTEST_CONFIGS=${TEST_CONFIGS}\"
		\"-Dtest=${test}\"
		\"-DRESOURCE_PATH=${RESOURCE_PATH}\"
		\"-DES_CONFIG=${ES_CONFIG}\"
		-DDEBUG=--debug
		-P \"${CMAKE_SOURCE_DIR}/integration/RunIntegrationTest.cmake\")")
	set(SET_TEST_PROPS_DEBUG
"set_tests_properties([==[[debug] ${test}]==] PROPERTIES
	WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"
	LABELS integration-debug)")
	set(TEST_SCRIPT ${TEST_SCRIPT}\n${ADD_TEST}\n${SET_TEST_PROPS}\n${ADD_TEST_DEBUG}\n${SET_TEST_PROPS_DEBUG}\n)
endforeach()

file(WRITE "${BINARY_PATH}/IntegrationTests_tests.cmake" "${TEST_SCRIPT}")
