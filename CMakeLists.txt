cmake_minimum_required(VERSION 3.16)
project(SystemRDLToolbox)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Find ANTLR4 runtime library
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(ANTLR4 REQUIRED antlr4-runtime)

# If pkg-config cannot find it, try manual search
if(NOT ANTLR4_FOUND)
    find_path(ANTLR4_INCLUDE_DIR
        NAMES antlr4-runtime.h
        PATHS
            /usr/include/antlr4-runtime
            /usr/local/include/antlr4-runtime
            /opt/antlr4/include/antlr4-runtime
    )

    find_library(ANTLR4_LIBRARY
        NAMES antlr4-runtime
        PATHS
            /usr/lib64
            /usr/local/lib64
            /opt/antlr4/lib64
            /usr/lib
            /usr/local/lib
            /opt/antlr4/lib
    )

    if(ANTLR4_INCLUDE_DIR AND ANTLR4_LIBRARY)
        set(ANTLR4_FOUND TRUE)
        set(ANTLR4_INCLUDE_DIRS ${ANTLR4_INCLUDE_DIR})
        set(ANTLR4_LIBRARIES ${ANTLR4_LIBRARY})
    endif()
endif()

if(NOT ANTLR4_FOUND)
    message(FATAL_ERROR "ANTLR4 runtime library not found. Please install antlr4-cpp-runtime-dev or similar package.")
endif()

# Add generated source files
set(GENERATED_SOURCES
    SystemRDLLexer.cpp
    SystemRDLParser.cpp
    SystemRDLBaseVisitor.cpp
    SystemRDLVisitor.cpp
)

# Add generated header files
set(GENERATED_HEADERS
    SystemRDLLexer.h
    SystemRDLParser.h
    SystemRDLBaseVisitor.h
    SystemRDLVisitor.h
)

# Add custom headers
set(CUSTOM_HEADERS
    cmdline_parser.h
    json_output.h
    elaborator.h
)

# Create AST printer executable
add_executable(systemrdl_parser
    parser_main.cpp
    ${GENERATED_SOURCES}
)

# Create Elaborator executable
add_executable(systemrdl_elaborator
    elaborator_main.cpp
    elaborator.cpp
    ${GENERATED_SOURCES}
)

# Set include directories
target_include_directories(systemrdl_parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANTLR4_INCLUDE_DIRS}
)

target_include_directories(systemrdl_elaborator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANTLR4_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(systemrdl_parser
    ${ANTLR4_LIBRARIES}
)

target_link_libraries(systemrdl_elaborator
    ${ANTLR4_LIBRARIES}
)

# Compile options
target_compile_options(systemrdl_parser PRIVATE
    ${ANTLR4_CFLAGS_OTHER}
)

target_compile_options(systemrdl_elaborator PRIVATE
    ${ANTLR4_CFLAGS_OTHER}
)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(systemrdl_parser PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
    )
    target_compile_options(systemrdl_elaborator PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
    )
endif()

# Install target (optional)
install(TARGETS systemrdl_parser systemrdl_elaborator
    RUNTIME DESTINATION bin
)

# Find all RDL test files
file(GLOB RDL_TEST_FILES "${CMAKE_SOURCE_DIR}/test/*.rdl")

# Create tests for parser
foreach(rdl_file ${RDL_TEST_FILES})
    get_filename_component(test_name ${rdl_file} NAME_WE)
    add_test(
        NAME "parser_${test_name}"
        COMMAND systemrdl_parser ${rdl_file}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties("parser_${test_name}" PROPERTIES
        LABELS "parser"
    )
endforeach()

# Create tests for elaborator
foreach(rdl_file ${RDL_TEST_FILES})
    get_filename_component(test_name ${rdl_file} NAME_WE)
    add_test(
        NAME "elaborator_${test_name}"
        COMMAND systemrdl_elaborator ${rdl_file}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties("elaborator_${test_name}" PROPERTIES
        LABELS "elaborator"
    )
endforeach()

# Find Python3 for JSON validation tests
# First try to find Python in virtual environment if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
    set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
    message(STATUS "Using Python from virtual environment: ${Python3_EXECUTABLE}")
else()
    find_package(Python3 COMPONENTS Interpreter)
endif()

# Check if we have a working Python3 interpreter
if(Python3_EXECUTABLE)
    set(Python3_FOUND TRUE)
    message(STATUS "Python3 executable: ${Python3_EXECUTABLE}")
else()
    set(Python3_FOUND FALSE)
    message(WARNING "Python3 not found")
endif()

# Create JSON output tests if Python3 is available
if(Python3_FOUND)
    foreach(rdl_file ${RDL_TEST_FILES})
        get_filename_component(test_name ${rdl_file} NAME_WE)

        # JSON test using unified Python script (replaces bash script)
        add_test(
            NAME "json_test_${test_name}"
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/json_output_validator.py
                    --test
                    --parser ${CMAKE_BINARY_DIR}/systemrdl_parser
                    --elaborator ${CMAKE_BINARY_DIR}/systemrdl_elaborator
                    --rdl ${rdl_file}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_tests_properties("json_test_${test_name}" PROPERTIES
            LABELS "json;test"
            DEPENDS "systemrdl_parser;systemrdl_elaborator"
        )
    endforeach()

    # RDL Semantic Validation Test - validates all RDL files with Python SystemRDL compiler
    add_test(
        NAME "rdl_semantic_validation"
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/rdl_semantic_validator.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties("rdl_semantic_validation" PROPERTIES
        LABELS "semantic;validation;rdl"
        TIMEOUT 60
    )

    # Create test groups for convenience
    add_custom_target(test-json
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "json"
        DEPENDS systemrdl_parser systemrdl_elaborator
        COMMENT "Running JSON output tests"
    )

    add_custom_target(test-semantic
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "semantic"
        COMMENT "Running RDL semantic validation tests"
    )

    add_custom_target(test-all
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS systemrdl_parser systemrdl_elaborator
        COMMENT "Running all tests"
    )

    add_custom_target(test-fast
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L json
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L semantic
        DEPENDS systemrdl_parser systemrdl_elaborator
        COMMENT "Running fast tests (JSON + semantic validation)"
    )
else()
    message(WARNING "Python3 not found - JSON and semantic validation tests will be disabled")
endif()

# Custom target to run all tests with nice output
add_custom_target(run-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS systemrdl_parser systemrdl_elaborator
    COMMENT "Running all SystemRDL tests..."
)

# Custom target to run only parser tests
add_custom_target(test-parser
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose -L parser
    DEPENDS systemrdl_parser
    COMMENT "Running SystemRDL parser tests..."
)

# Custom target to run only elaborator tests
add_custom_target(test-elaborator
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose -L elaborator
    DEPENDS systemrdl_elaborator
    COMMENT "Running SystemRDL elaborator tests..."
)

# Print configuration information
message(STATUS "ANTLR4 include dirs: ${ANTLR4_INCLUDE_DIRS}")
message(STATUS "ANTLR4 libraries: ${ANTLR4_LIBRARIES}")
message(STATUS "Found ${list_length} RDL test files")

# Print found test files
list(LENGTH RDL_TEST_FILES test_files_count)
message(STATUS "Found ${test_files_count} RDL test files:")
foreach(rdl_file ${RDL_TEST_FILES})
    get_filename_component(test_name ${rdl_file} NAME)
    message(STATUS "  - ${test_name}")
endforeach()
