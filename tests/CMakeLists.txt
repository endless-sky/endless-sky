add_executable(EndlessSkyTests)
if(UNIX AND NOT APPLE)
	set_target_properties(EndlessSkyTests PROPERTIES OUTPUT_NAME "endless-sky-tests")
endif()

if(NOT MSVC)
	target_compile_options(EndlessSkyTests PUBLIC
		"-Wall" "-pedantic-errors" "-Wold-style-cast" "-fno-rtti")
endif()

find_package(Catch2 CONFIG REQUIRED)

# Every source file (and header file) should be listed here.
# If you add a new file, add it to this list.
target_sources(EndlessSkyTests PRIVATE
	unit/include/datanode-factory.h
	unit/include/es-test.hpp
	unit/include/logger-output.h
	unit/include/output-capture.hpp
	unit/src/comparators/test_byGivenOrder.cpp
	unit/src/comparators/test_byName.cpp
	unit/src/helpers/datanode-factory.cpp
	unit/src/helpers/logger-output.cpp
	unit/src/test_account.cpp
	unit/src/test_angle.cpp
	unit/src/test_bitset.cpp
	unit/src/test_categoryList.cpp
	unit/src/test_conditionAssignments.cpp
	unit/src/test_conditionSet.cpp
	unit/src/test_conditionsStore.cpp
	unit/src/test_datafile.cpp
	unit/src/test_datanode.cpp
	unit/src/test_datawriter.cpp
	unit/src/test_dictionary.cpp
	unit/src/test_distance_calculation_settings.cpp
	unit/src/test_esuuid.cpp
	unit/src/test_exclusiveItem.cpp
	unit/src/test_firecommand.cpp
	unit/src/test_formationPattern.cpp
	unit/src/test_main.cpp
	unit/src/test_point.cpp
	unit/src/test_random.cpp
	unit/src/test_scrollVar.cpp
	unit/src/test_set.cpp
	unit/src/test_ship.cpp
	unit/src/test_stringInterner.cpp
	unit/src/test_template.txt
	unit/src/test_weightedList.cpp
	unit/src/text/test_alignment.cpp
	unit/src/text/test_displaytext.cpp
	unit/src/text/test_format.cpp
	unit/src/text/test_layout.cpp
	unit/src/text/test_truncate.cpp
)

list(APPEND INTEGRATION_TESTS
	integration/config/plugins/integration-tests/data/tests/tests_afterburn_flight.txt
	integration/config/plugins/integration-tests/data/tests/tests_capture_override.txt
	integration/config/plugins/integration-tests/data/tests/tests_common.txt
	integration/config/plugins/integration-tests/data/tests/tests_conditional_choice.txt
	integration/config/plugins/integration-tests/data/tests/tests_disown.txt
	integration/config/plugins/integration-tests/data/tests/tests_flagship_attribute_autoconditions.txt
	integration/config/plugins/integration-tests/data/tests/tests_flagship_model_condition.txt
	integration/config/plugins/integration-tests/data/tests/tests_flagship_planet_attribute.txt
	integration/config/plugins/integration-tests/data/tests/tests_framework.txt
	integration/config/plugins/integration-tests/data/tests/tests_hyperjumps_to_autocondition.txt
	integration/config/plugins/integration-tests/data/tests/tests_illegal_atrocity.txt
	integration/config/plugins/integration-tests/data/tests/tests_job_board_missions.txt
	integration/config/plugins/integration-tests/data/tests/tests_landing.txt
	integration/config/plugins/integration-tests/data/tests/tests_take_ships.txt
	integration/config/plugins/integration-tests/data/tests/tests_timers.txt
	integration/config/plugins/integration-tests/data/tests/tests_save_load.txt
	integration/config/plugins/integration-tests/data/tests/tests_ship_sale.txt
	integration/config/plugins/integration-tests/data/tests/tests_shipyard_outfitter_missions.txt
	integration/config/plugins/integration-tests/data/tests/tests_store_outfits_on_take_off.txt
	integration/config/plugins/integration-tests/data/tests/tests_to_accept.txt
	integration/config/plugins/integration-tests/data/tests/tests_wormhole_navigation.txt
	integration/config/plugins/integration-tests/data/tests/tests_installed_plugin.txt
)

target_include_directories(EndlessSkyTests PRIVATE unit/include)
target_link_libraries(EndlessSkyTests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(EndlessSkyTests PRIVATE ExternalLibraries $<TARGET_OBJECTS:EndlessSkyLib>)

target_compile_options(EndlessSkyTests PUBLIC ${SANITIZER_OPTS})
target_link_options(EndlessSkyTests PUBLIC ${SANITIZER_OPTS})

# CTest support for our unit tests.
add_test(NAME unit COMMAND EndlessSkyTests)
set_tests_properties(unit PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" LABELS unit)
add_test(NAME benchmark COMMAND "$<TARGET_FILE:EndlessSkyTests>" [!benchmark])
set_tests_properties(benchmark PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" LABELS benchmark)

# Integration tests.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/IntegrationTests_tests.cmake"
	COMMENT "Discover every integration test"
	COMMAND "${CMAKE_COMMAND}" -DES=$<TARGET_FILE:EndlessSky>
		-DBINARY_PATH=${CMAKE_CURRENT_BINARY_DIR}
		-DRESOURCE_PATH=${CMAKE_SOURCE_DIR}
		-DES_USE_OFFSCREEN=${ES_USE_OFFSCREEN}
		-P "${CMAKE_CURRENT_SOURCE_DIR}/integration/IntegrationTests.cmake"
	DEPENDS EndlessSky ${INTEGRATION_TESTS}
		"${CMAKE_CURRENT_SOURCE_DIR}/integration/IntegrationTests.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${CMAKE_CURRENT_BINARY_DIR}/IntegrationTests_tests.cmake")
add_custom_target(IntegrationTests ALL
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/IntegrationTests_tests.cmake"
	VERBATIM
)

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/IntegrationTests_tests.cmake")
	# If the target is not run, simply don't provide any integration tests instead of erroring out.
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/IntegrationTests_tests.cmake" "")
endif()

# Copy the MinGW runtime DLLs if necessary.
if(MINGW AND WIN32)
	foreach(FILE_PATH ${MINGW_RUNTIME})
		add_custom_command(TARGET EndlessSkyTests POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${FILE_PATH}" "$<TARGET_FILE_DIR:EndlessSkyTests>"
			COMMAND_EXPAND_LISTS VERBATIM)
	endforeach()
endif()
