set(ES_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/integration/config")

# Get all the tests to run.
execute_process(
	COMMAND "${ES}" --config "${ES_CONFIG}" --tests
	OUTPUT_VARIABLE INTEGRATION_TESTS
)

string(REPLACE "\n" ";" INTEGRATION_TESTS_LIST "${INTEGRATION_TESTS}")

# Empty the temp config directory.
set(TEST_CONFIGS "${BINARY_PATH}/integration_configs")
set(TEST_SCRIPT "file(REMOVE_RECURSE \"${TEST_CONFIGS}\")")
set(TEST_SCRIPT "${TEST_SCRIPT}\nfile(MAKE_DIRECTORY \"${TEST_CONFIGS}\")\n")

# Choose between using the offscreen backend and Xvfb.
if(ES_USE_OFFSCREEN)
	set(OFFSCREEN "set(ENV{SDL_VIDEODRIVER} offscreen)\n")
else()
	set(XFVB "xvfb-run --auto-servernum \"--server-args=+extension GLX +render -noreset\"")
endif()

# Add each test as CTest.
foreach(test ${INTEGRATION_TESTS_LIST})
	# Copy the template config
	set(TEST_CONFIG "${TEST_CONFIGS}/${test}")
	set(COPY_CONFIG "file(COPY \"${ES_CONFIG}\" DESTINATION \"${TEST_CONFIGS}\")")
	set(RENAME_CONFIG "file(RENAME \"${TEST_CONFIGS}/config\" \"${TEST_CONFIG}\")")

	set(ADD_TEST
"add_test([==[${test}]==]
	$ENV{ES_INTEGRATION_PREFIX} ${XFVB} \"${ES}\" --config \"${TEST_CONFIG}\" --test \"${test}\")")
	set(SET_TEST_PROPS
"set_tests_properties([==[${test}]==] PROPERTIES
	WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"
	LABELS integration)")
	set(TEST_SCRIPT ${TEST_SCRIPT}${COPY_CONFIG}\n${RENAME_CONFIG}\n${ADD_TEST}\n${SET_TEST_PROPS}\n)
endforeach()

file(WRITE "${BINARY_PATH}/IntegrationTests_tests.cmake" "${OFFSCREEN}${TEST_SCRIPT}")
