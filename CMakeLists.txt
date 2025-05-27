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

# Create CSV to RDL converter executable
add_executable(systemrdl_csv2rdl
    csv2rdl_main.cpp
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

target_include_directories(systemrdl_csv2rdl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(systemrdl_parser
    ${ANTLR4_LIBRARIES}
)

target_link_libraries(systemrdl_elaborator
    ${ANTLR4_LIBRARIES}
)

# CSV converter doesn't need ANTLR4
# target_link_libraries(systemrdl_csv2rdl)

# Add dependency on ANTLR4 target when using downloaded version
if(NOT USE_SYSTEM_ANTLR4)
    add_dependencies(systemrdl_parser ${ANTLR4_TARGET})
    add_dependencies(systemrdl_elaborator ${ANTLR4_TARGET})
    # systemrdl_csv2rdl doesn't depend on ANTLR4
endif()

# Compile options
target_compile_options(systemrdl_parser PRIVATE
    ${ANTLR4_CFLAGS_OTHER}
)

target_compile_options(systemrdl_elaborator PRIVATE
    ${ANTLR4_CFLAGS_OTHER}
)

# CSV converter doesn't need ANTLR4 compile options
# target_compile_options(systemrdl_csv2rdl PRIVATE)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(systemrdl_parser PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
    )
    target_compile_options(systemrdl_elaborator PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
    )
    target_compile_options(systemrdl_csv2rdl PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
    )
endif()

# Install target (optional)
install(TARGETS systemrdl_parser systemrdl_elaborator systemrdl_csv2rdl
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

# Find all CSV test files
file(GLOB CSV_TEST_FILES "${CMAKE_SOURCE_DIR}/test/test_csv_*.csv")

# Create tests for CSV converter
foreach(csv_file ${CSV_TEST_FILES})
    get_filename_component(test_name ${csv_file} NAME_WE)
    add_test(
        NAME "csv2rdl_${test_name}"
        COMMAND systemrdl_csv2rdl ${csv_file} -o ${test_name}.rdl
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties("csv2rdl_${test_name}" PROPERTIES
        LABELS "csv2rdl"
    )
endforeach()

# Find Python and markdown linting tools
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
    # Use virtual environment Python if available
    set(PYTHON_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
    message(STATUS "Using Python from virtual environment: ${PYTHON_EXECUTABLE}")
else()
    find_program(PYTHON_EXECUTABLE
        NAMES python3 python
        DOC "Path to Python executable"
    )
endif()

# Check if we have a working Python3 interpreter
if(PYTHON_EXECUTABLE)
    set(Python3_FOUND TRUE)
    message(STATUS "Python3 executable: ${PYTHON_EXECUTABLE}")
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
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/json_output_validator.py
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
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/rdl_semantic_validator.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties("rdl_semantic_validation" PROPERTIES
        LABELS "semantic;validation;rdl"
        TIMEOUT 60
    )

    # CSV2RDL Validation Test - validates CSV converter with comprehensive test suite
    add_test(
        NAME "csv2rdl_validation"
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/csv2rdl_validator.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties("csv2rdl_validation" PROPERTIES
        LABELS "csv2rdl;validation"
        TIMEOUT 120
        DEPENDS "systemrdl_csv2rdl;systemrdl_parser"
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

    add_custom_target(test-csv2rdl
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "csv2rdl"
        DEPENDS systemrdl_csv2rdl systemrdl_parser
        COMMENT "Running CSV2RDL converter tests"
    )

    add_custom_target(test-fast
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L json
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L semantic
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L csv2rdl
        DEPENDS systemrdl_parser systemrdl_elaborator systemrdl_csv2rdl
        COMMENT "Running fast tests (JSON + semantic + CSV2RDL validation)"
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

list(LENGTH CSV_TEST_FILES csv_test_files_count)
message(STATUS "Found ${csv_test_files_count} CSV test files:")
foreach(csv_file ${CSV_TEST_FILES})
    get_filename_component(test_name ${csv_file} NAME)
    message(STATUS "  - ${test_name}")
endforeach()

# ==============================================================================
# Code Quality and Formatting Targets
# ==============================================================================

# Find required tools for code quality checks
find_program(CLANG_FORMAT_EXECUTABLE
    NAMES clang-format clang-format-14 clang-format-13 clang-format-12
    DOC "Path to clang-format executable"
)

find_program(CPPCHECK_EXECUTABLE
    NAMES cppcheck
    DOC "Path to cppcheck executable"
)

# Check for Python markdown linting tools
if(PYTHON_EXECUTABLE)
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "import pymarkdown; print('pymarkdown available')"
        RESULT_VARIABLE PYMARKDOWN_CHECK_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(PYMARKDOWN_CHECK_RESULT EQUAL 0)
        set(PYMARKDOWN_AVAILABLE TRUE)
    else()
        set(PYMARKDOWN_AVAILABLE FALSE)
    endif()
else()
    set(PYMARKDOWN_AVAILABLE FALSE)
endif()

# Collect source files for quality checks
file(GLOB_RECURSE CPP_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/elaborator.cpp"
    "${CMAKE_SOURCE_DIR}/elaborator_main.cpp"
    "${CMAKE_SOURCE_DIR}/parser_main.cpp"
    "${CMAKE_SOURCE_DIR}/csv2rdl_main.cpp"
    "${CMAKE_SOURCE_DIR}/*.h"
)

# Filter out build directory, generated files, and other unwanted files
list(FILTER CPP_SOURCE_FILES EXCLUDE REGEX ".*/(build|\.git|\.venv|__pycache__)/.+")
list(FILTER CPP_SOURCE_FILES EXCLUDE REGEX ".*/SystemRDL(Lexer|Parser|BaseVisitor|Visitor)\.(h|cpp)$")

# Collect Python files for quality checks
file(GLOB_RECURSE PYTHON_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/script/*.py"
)

# Filter out unwanted python files
list(FILTER PYTHON_SOURCE_FILES EXCLUDE REGEX ".*/(build|\.git|\.venv|__pycache__)/.+")

# Collect Markdown files for linting
file(GLOB_RECURSE MARKDOWN_FILES
    "${CMAKE_SOURCE_DIR}/*.md"
    "${CMAKE_SOURCE_DIR}/docs/*.md"
)

# Filter out unwanted markdown files - exclude build artifacts, git files,
# virtual environments, cache directories, GitHub templates, and test data
list(FILTER MARKDOWN_FILES EXCLUDE REGEX ".*/(build|\.git|\.venv|__pycache__|\.github|test)/.*")

# Also exclude specific files that may have special formatting requirements
list(FILTER MARKDOWN_FILES EXCLUDE REGEX ".*/CHANGELOG\.md$")
list(FILTER MARKDOWN_FILES EXCLUDE REGEX ".*/UPDATED_.*\.md$")

# ==============================================================================
# C++ Code Quality Targets
# ==============================================================================

if(CLANG_FORMAT_EXECUTABLE)
    # Check C++ code formatting
    add_custom_target(format-check
        COMMAND echo "Checking C++ code formatting..."
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --version
        COMMAND echo "Using .clang-format configuration from project root"
        COMMAND ${CLANG_FORMAT_EXECUTABLE}
            --style=file:${CMAKE_SOURCE_DIR}/.clang-format
            --dry-run
            -Werror
            ${CPP_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking C++ code formatting with clang-format"
    )

    # Show C++ formatting differences
    add_custom_target(format-diff
        COMMAND echo "Showing C++ formatting differences..."
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --version
        COMMAND ${CLANG_FORMAT_EXECUTABLE}
            --style=file:${CMAKE_SOURCE_DIR}/.clang-format
            --diff
            ${CPP_SOURCE_FILES} || echo "Files shown above need formatting"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Showing C++ formatting differences"
    )

    # Auto-format C++ code
    add_custom_target(format
        COMMAND echo "Auto-formatting C++ code..."
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --version
        COMMAND ${CLANG_FORMAT_EXECUTABLE}
            --style=file:${CMAKE_SOURCE_DIR}/.clang-format
            -i
            ${CPP_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Auto-formatting C++ code with clang-format"
    )
else()
    # Fallback targets when clang-format is not available
    add_custom_target(format-check
        COMMAND echo "❌ clang-format not found! Please install clang-format to check C++ formatting."
        COMMAND echo "   Ubuntu/Debian: sudo apt-get install clang-format"
        COMMAND echo "   Gentoo: sudo emerge clang"
        COMMAND false
    )

    add_custom_target(format-diff
        COMMAND echo "❌ clang-format not found! Please install clang-format to show formatting differences."
        COMMAND false
    )

    add_custom_target(format
        COMMAND echo "❌ clang-format not found! Please install clang-format to auto-format C++ code."
        COMMAND false
    )
endif()

if(CPPCHECK_EXECUTABLE)
    # Run cppcheck static analysis
    add_custom_target(cppcheck
        COMMAND echo "Running cppcheck static analysis..."
        COMMAND ${CPPCHECK_EXECUTABLE} --version
        COMMAND ${CPPCHECK_EXECUTABLE}
            --language=c++
            --enable=warning,style,performance
            --suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppressions
            --inline-suppr
            --quiet
            ${CPP_SOURCE_FILES} || echo "Note: Some informational messages may appear above"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis"
    )

    # Run cppcheck with verbose output
    add_custom_target(cppcheck-verbose
        COMMAND echo "Running cppcheck static analysis (verbose)..."
        COMMAND ${CPPCHECK_EXECUTABLE} --version
        COMMAND ${CPPCHECK_EXECUTABLE}
            --enable=warning,style,performance
            --suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppressions
            --inline-suppr
            --verbose
            ${CPP_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis (verbose)"
    )
else()
    # Fallback targets when cppcheck is not available
    add_custom_target(cppcheck
        COMMAND echo "❌ cppcheck not found! Please install cppcheck for static analysis."
        COMMAND echo "   Ubuntu/Debian: sudo apt-get install cppcheck"
        COMMAND echo "   Gentoo: sudo emerge dev-util/cppcheck"
        COMMAND false
    )

    add_custom_target(cppcheck-verbose
        COMMAND echo "❌ cppcheck not found! Please install cppcheck for static analysis."
        COMMAND false
    )
endif()

# ==============================================================================
# Python Code Quality Targets
# ==============================================================================

if(PYTHON_EXECUTABLE)
    # Check Python code formatting
    add_custom_target(python-format-check
        COMMAND ${CMAKE_COMMAND} -E echo "Checking Python code formatting..."
        COMMAND ${PYTHON_EXECUTABLE} -m black --check --diff ${PYTHON_SOURCE_FILES}
        COMMAND ${PYTHON_EXECUTABLE} -m isort --check-only --diff ${PYTHON_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking Python code formatting"
    )

    # Auto-format Python code
    add_custom_target(python-format
        COMMAND ${CMAKE_COMMAND} -E echo "Auto-formatting Python code..."
        COMMAND ${PYTHON_EXECUTABLE} -m black ${PYTHON_SOURCE_FILES}
        COMMAND ${PYTHON_EXECUTABLE} -m isort ${PYTHON_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Auto-formatting Python code with black and isort"
    )

    # Run Python linting
    add_custom_target(python-lint
        COMMAND ${CMAKE_COMMAND} -E echo "Running Python linting..."
        COMMAND ${PYTHON_EXECUTABLE} -m flake8 ${PYTHON_SOURCE_FILES} --count --exit-zero --max-line-length=127 --ignore=C901,W503,E226 --statistics || ${CMAKE_COMMAND} -E echo "Python linting completed"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running Python linting with flake8"
    )

    # Complete Python quality check
    add_custom_target(python-quality
        COMMAND echo "Running complete Python quality checks..."
        DEPENDS python-format-check python-lint
        COMMENT "Running complete Python quality checks"
    )
else()
    # Fallback targets when Python is not available
    add_custom_target(python-format-check
        COMMAND echo "❌ Python not found! Please install Python to check Python code quality."
        COMMAND false
    )

    add_custom_target(python-format
        COMMAND echo "❌ Python not found! Please install Python to format Python code."
        COMMAND false
    )

    add_custom_target(python-lint
        COMMAND echo "❌ Python not found! Please install Python to lint Python code."
        COMMAND false
    )

    add_custom_target(python-quality
        COMMAND echo "❌ Python not found! Please install Python to check Python code quality."
        COMMAND false
    )
endif()

# ==============================================================================
# Markdown Linting Targets
# ==============================================================================

if(PYMARKDOWN_AVAILABLE)
    # Check markdown formatting and style
    add_custom_target(markdown-lint
        COMMAND echo "Running markdown linting..."
        COMMAND ${PYTHON_EXECUTABLE} -c "import pymarkdown"
        COMMAND ${PYTHON_EXECUTABLE} -m pymarkdown --config ${CMAKE_SOURCE_DIR}/.pymarkdown.json scan ${MARKDOWN_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Linting Markdown files with PyMarkdown"
    )

    # Auto-fix markdown issues where possible
    add_custom_target(markdown-fix
        COMMAND echo "Auto-fixing markdown issues..."
        COMMAND ${PYTHON_EXECUTABLE} -m pymarkdown --config ${CMAKE_SOURCE_DIR}/.pymarkdown.json fix ${MARKDOWN_FILES} || echo "Some issues may require manual fixing"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Auto-fixing Markdown issues with PyMarkdown"
    )

    # Show markdown linting rules
    add_custom_target(markdown-rules
        COMMAND echo "Available markdown linting rules:"
        COMMAND ${PYTHON_EXECUTABLE} -m pymarkdown plugins list
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Showing available markdown linting rules"
    )
else()
    # Fallback targets when pymarkdown is not available
    add_custom_target(markdown-lint
        COMMAND echo "❌ PyMarkdown not found! Please install it to lint Markdown files."
        COMMAND echo "   Install with: pip install pymarkdown"
        COMMAND echo "   Or use requirements.txt: pip install -r requirements.txt"
        COMMAND false
    )

    add_custom_target(markdown-fix
        COMMAND echo "❌ PyMarkdown not found! Please install it to fix Markdown files."
        COMMAND false
    )

    add_custom_target(markdown-rules
        COMMAND echo "❌ PyMarkdown not found! Please install it to see markdown rules."
        COMMAND false
    )
endif()

# ==============================================================================
# Combined Quality Targets
# ==============================================================================

# Check all code quality (no auto-fixes)
add_custom_target(quality-check
    COMMAND echo "Running all quality checks..."
    DEPENDS format-check cppcheck python-quality markdown-lint
    COMMENT "Running all quality checks (C++, Python, Markdown)"
)

# Auto-format all code
add_custom_target(quality-format
    COMMAND echo "Auto-formatting all code..."
    DEPENDS format python-format markdown-fix
    COMMENT "Auto-formatting all code (C++, Python, Markdown)"
)

# Fix all auto-fixable issues
add_custom_target(fix-all
    COMMAND echo "Fixing all auto-fixable issues..."
    DEPENDS quality-format
    COMMENT "Auto-fixing all code quality issues"
)

# Comprehensive quality analysis with verbose output
add_custom_target(quality-all
    COMMAND echo "Running comprehensive quality analysis..."
    DEPENDS format-diff cppcheck-verbose python-quality markdown-lint
    COMMENT "Running comprehensive quality analysis"
)

# Pre-commit style checks (fast)
add_custom_target(pre-commit
    COMMAND echo "Running pre-commit checks..."
    DEPENDS quality-check test-fast
    COMMENT "Running pre-commit checks (quality + fast tests)"
)

# ==============================================================================
# Help target
# ==============================================================================

add_custom_target(quality-help
    COMMAND echo ""
    COMMAND echo "📋 Available Code Quality Targets:"
    COMMAND echo "=================================="
    COMMAND echo ""
    COMMAND echo "🔍 Code Checking:"
    COMMAND echo "  quality-check      - Run all quality checks"
    COMMAND echo "  format-check       - Check C++ code formatting"
    COMMAND echo "  format-diff        - Show C++ formatting differences"
    COMMAND echo "  cppcheck           - Run C++ static analysis"
    COMMAND echo "  cppcheck-verbose   - Run C++ static analysis verbose"
    COMMAND echo "  python-quality     - Run Python quality checks"
    COMMAND echo "  python-format-check - Check Python code formatting"
    COMMAND echo "  python-lint        - Run Python linting"
    COMMAND echo "  markdown-lint      - Lint Markdown files"
    COMMAND echo "  markdown-rules     - Show available markdown rules"
    COMMAND echo ""
    COMMAND echo "🔧 Code Fixing:"
    COMMAND echo "  quality-format     - Auto-format all code"
    COMMAND echo "  format             - Auto-format C++ code"
    COMMAND echo "  python-format      - Auto-format Python code"
    COMMAND echo "  markdown-fix       - Auto-fix Markdown issues"
    COMMAND echo "  fix-all            - Fix all auto-fixable issues"
    COMMAND echo ""
    COMMAND echo "🚀 Combined Targets:"
    COMMAND echo "  pre-commit         - Run pre-commit checks"
    COMMAND echo "  quality-all        - Run complete quality analysis"
    COMMAND echo ""
    COMMAND echo "💡 Examples:"
    COMMAND echo "  make quality-check    # Check code quality"
    COMMAND echo "  make format           # Auto-format C++ code"
    COMMAND echo "  make markdown-lint    # Check Markdown files"
    COMMAND echo "  make pre-commit       # Run what CI will check"
    COMMAND echo ""
    COMMENT "Showing code quality help"
)
