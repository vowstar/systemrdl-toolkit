cmake_minimum_required(VERSION 3.16)
project(SystemRDLToolbox)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Include FetchContent module for downloading dependencies
include(FetchContent)

# Option to control ANTLR4 usage
option(USE_SYSTEM_ANTLR4 "Use system-installed ANTLR4 C++ runtime" OFF)

# ANTLR4 version to use when not using system version
# Can be overridden by environment variable or command line
if(NOT ANTLR4_VERSION)
    if(DEFINED ENV{ANTLR4_VERSION})
        set(ANTLR4_VERSION $ENV{ANTLR4_VERSION})
        message(STATUS "Using ANTLR4 version from environment variable: ${ANTLR4_VERSION}")
    else()
        set(ANTLR4_VERSION "4.13.2")
        message(STATUS "Using default ANTLR4 version: ${ANTLR4_VERSION}")
    endif()
else()
    message(STATUS "Using ANTLR4 version from command line: ${ANTLR4_VERSION}")
endif()

# Print version info
message(STATUS "ANTLR4 version: ${ANTLR4_VERSION}")

if(USE_SYSTEM_ANTLR4)
    # Use system ANTLR4 - existing logic
    message(STATUS "Using system ANTLR4 C++ runtime")

    # Find ANTLR4 runtime library
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ANTLR4 antlr4-runtime)

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
        message(FATAL_ERROR "ANTLR4 runtime library not found. Please install antlr4-cpp-runtime-dev or similar package, or set USE_SYSTEM_ANTLR4=OFF to download automatically.")
    endif()

    # Set variables for compatibility
    set(ANTLR4_TARGET antlr4_static)

else()
    # Download and build ANTLR4 C++ runtime
    message(STATUS "Downloading ANTLR4 C++ runtime version ${ANTLR4_VERSION}")

    # Configure ANTLR4 build options before FetchContent - disable all unnecessary components
    set(ANTLR4_BUILD_CPP_TESTS OFF CACHE BOOL "Build ANTLR4 C++ tests" FORCE)
    set(ANTLR4_INSTALL OFF CACHE BOOL "Install ANTLR4 C++ runtime" FORCE)
    set(WITH_DEMO OFF CACHE BOOL "Build ANTLR4 demo" FORCE)
    set(WITH_LIBCXX OFF CACHE BOOL "Build with libc++" FORCE)
    set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "Build ANTLR C++ tests" FORCE)

    FetchContent_Declare(
        antlr4_runtime
        URL https://github.com/antlr/antlr4/archive/refs/tags/${ANTLR4_VERSION}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR runtime/Cpp
    )

    FetchContent_MakeAvailable(antlr4_runtime)

    # Set variables for downloaded version
    set(ANTLR4_INCLUDE_DIRS ${antlr4_runtime_SOURCE_DIR}/runtime/Cpp/runtime/src)
    set(ANTLR4_LIBRARIES antlr4_static)
    set(ANTLR4_TARGET antlr4_static)
    set(ANTLR4_FOUND TRUE)
endif()

# Custom target to download ANTLR4 JAR file (not executed by default)
set(ANTLR4_JAR_PATH ${CMAKE_BINARY_DIR}/antlr4-${ANTLR4_VERSION}-complete.jar)
set(ANTLR4_DOWNLOAD_URL https://repo1.maven.org/maven2/org/antlr/antlr4/${ANTLR4_VERSION}/antlr4-${ANTLR4_VERSION}-complete.jar)

add_custom_command(
    OUTPUT ${ANTLR4_JAR_PATH}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}
    COMMAND curl -L -o ${ANTLR4_JAR_PATH} ${ANTLR4_DOWNLOAD_URL}
    COMMENT "Downloading ANTLR4 JAR ${ANTLR4_VERSION} from ${ANTLR4_DOWNLOAD_URL}"
)

add_custom_target(download-antlr4-jar
    DEPENDS ${ANTLR4_JAR_PATH}
    COMMENT "Download ANTLR4 JAR file for code generation"
)

# Custom target to generate C++ files from grammar (not executed by default)
add_custom_target(generate-antlr4-cpp
    COMMAND java -jar ${ANTLR4_JAR_PATH} -Dlanguage=Cpp -visitor -no-listener 
        SystemRDL.g4
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS download-antlr4-jar
    COMMENT "Generating C++ files from SystemRDL.g4 grammar"
)

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

# Add dependency on ANTLR4 target when using downloaded version
if(NOT USE_SYSTEM_ANTLR4)
    add_dependencies(systemrdl_parser ${ANTLR4_TARGET})
    add_dependencies(systemrdl_elaborator ${ANTLR4_TARGET})
endif()

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
