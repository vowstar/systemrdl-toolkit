cmake_minimum_required(VERSION 3.16)

# ==============================================================================
# Version Management
# ==============================================================================

# Extract version from header file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/systemrdl_version.h" VERSION_H_CONTENT)
string(REGEX MATCH "#define SYSTEMRDL_VERSION_MAJOR ([0-9]+)" _ ${VERSION_H_CONTENT})
set(SYSTEMRDL_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define SYSTEMRDL_VERSION_MINOR ([0-9]+)" _ ${VERSION_H_CONTENT})
set(SYSTEMRDL_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define SYSTEMRDL_VERSION_PATCH ([0-9]+)" _ ${VERSION_H_CONTENT})
set(SYSTEMRDL_VERSION_PATCH ${CMAKE_MATCH_1})

set(SYSTEMRDL_VERSION "${SYSTEMRDL_VERSION_MAJOR}.${SYSTEMRDL_VERSION_MINOR}.${SYSTEMRDL_VERSION_PATCH}")
message(STATUS "Extracted version from header: ${SYSTEMRDL_VERSION}")

# Set up project with extracted version
project(SystemRDL-Toolkit
    VERSION ${SYSTEMRDL_VERSION}
    DESCRIPTION "SystemRDL parsing and elaboration toolkit library"
    LANGUAGES CXX
)

# ==============================================================================
# Git Information
# ==============================================================================

# Find Git (optional)
find_package(Git QUIET)

if(GIT_FOUND)
    # Get current commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Get current branch name
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH_NAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Check if working directory is clean
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_IS_DIRTY
        ERROR_QUIET
    )

    if(GIT_IS_DIRTY)
        set(GIT_COMMIT_HASH "${GIT_COMMIT_HASH}-dirty")
    endif()

    message(STATUS "Git information: ${GIT_BRANCH_NAME}@${GIT_COMMIT_HASH}")
else()
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_BRANCH_NAME "unknown")
    message(STATUS "Git not found - using default git information")
endif()

# ==============================================================================
# Main Project Detection
# ==============================================================================

# Determine if SystemRDL-Toolkit is built as a subproject (using add_subdirectory)
# or if it is the main project.
if(NOT DEFINED SYSTEMRDL_MAIN_PROJECT)
    set(SYSTEMRDL_MAIN_PROJECT OFF)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(SYSTEMRDL_MAIN_PROJECT ON)
        message(STATUS "SystemRDL-Toolkit is the main project")
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    else()
        message(STATUS "SystemRDL-Toolkit is being built as a subproject")
    endif()
endif()

# Protect against in-tree builds.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. You may need to delete "
                        "'CMakeCache.txt' and 'CMakeFiles/' before building again.")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include GNUInstallDirs for standard paths
include(GNUInstallDirs)

# ==============================================================================
# Build Options
# ==============================================================================

# Add options for building components
# When SystemRDL is used as a subproject, disable tools and tests by default to reduce build time
option(SYSTEMRDL_BUILD_TOOLS "Build command-line tools" ${SYSTEMRDL_MAIN_PROJECT})
option(SYSTEMRDL_BUILD_TESTS "Build tests" ${SYSTEMRDL_MAIN_PROJECT})
option(SYSTEMRDL_BUILD_SHARED "Build shared library" ON)
option(SYSTEMRDL_BUILD_STATIC "Build static library" ON)

# Print configuration information
if(SYSTEMRDL_MAIN_PROJECT)
    message(STATUS "SystemRDL-Toolkit main project build configuration:")
else()
    message(STATUS "SystemRDL-Toolkit subproject build configuration:")
endif()
message(STATUS "  Build shared library: ${SYSTEMRDL_BUILD_SHARED}")
message(STATUS "  Build static library: ${SYSTEMRDL_BUILD_STATIC}")
message(STATUS "  Build command-line tools: ${SYSTEMRDL_BUILD_TOOLS}")
message(STATUS "  Build tests: ${SYSTEMRDL_BUILD_TESTS}")

# Enable testing if requested
if(SYSTEMRDL_BUILD_TESTS)
    enable_testing()
endif()

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
    # Enable position independent code for shared library compatibility
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable PIC for ANTLR4" FORCE)

    # Configure ANTLR4 library type based on platform needs
    # On Windows, we need shared ANTLR4 for shared SystemRDL to avoid DLL import/export issues
    # On Unix, we prefer static ANTLR4 to avoid the mixed static/shared runtime issues we fixed

    # Preserve the original BUILD_SHARED_LIBS setting
    if(DEFINED BUILD_SHARED_LIBS)
        set(ORIGINAL_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    else()
        set(ORIGINAL_BUILD_SHARED_LIBS OFF)
    endif()

    if(WIN32 AND SYSTEMRDL_BUILD_SHARED)
        # On Windows, if we're building shared SystemRDL, we need shared ANTLR4
        set(BUILD_SHARED_LIBS ON)
        message(STATUS "Configuring ANTLR4 to build shared library for Windows DLL compatibility")
    else()
        # On Unix or if only building static SystemRDL, use static ANTLR4
        set(BUILD_SHARED_LIBS OFF)
        message(STATUS "Configuring ANTLR4 to build static library")
    endif()

    FetchContent_Declare(
        antlr4_runtime
        URL https://github.com/antlr/antlr4/archive/refs/tags/${ANTLR4_VERSION}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR runtime/Cpp
    )

    FetchContent_MakeAvailable(antlr4_runtime)

    # Restore the original BUILD_SHARED_LIBS setting
    set(BUILD_SHARED_LIBS ${ORIGINAL_BUILD_SHARED_LIBS})

    # Set variables for downloaded version
    set(ANTLR4_INCLUDE_DIRS ${antlr4_runtime_SOURCE_DIR}/runtime/Cpp/runtime/src)
    if(WIN32 AND SYSTEMRDL_BUILD_SHARED)
        # On Windows with shared SystemRDL, use shared ANTLR4
        set(ANTLR4_LIBRARIES antlr4_shared)
        set(ANTLR4_TARGET antlr4_shared)
        message(STATUS "Using shared ANTLR4 library for Windows DLL compatibility")
    else()
        # Otherwise use static ANTLR4
        set(ANTLR4_LIBRARIES antlr4_static)
        set(ANTLR4_TARGET antlr4_static)
        message(STATUS "Using static ANTLR4 library")
    endif()
    set(ANTLR4_SHARED_TARGET antlr4_shared)
    set(ANTLR4_FOUND TRUE)
endif()

# ==============================================================================
# nlohmann/json Library (Header-only) - Must be configured before Inja
# ==============================================================================

# Option to control nlohmann/json usage
option(USE_SYSTEM_NLOHMANN_JSON "Use system-installed nlohmann/json library" OFF)

# nlohmann/json version to use when not using system version
if(NOT NLOHMANN_JSON_VERSION)
    if(DEFINED ENV{NLOHMANN_JSON_VERSION})
        set(NLOHMANN_JSON_VERSION $ENV{NLOHMANN_JSON_VERSION})
        message(STATUS "Using nlohmann/json version from environment variable: ${NLOHMANN_JSON_VERSION}")
    else()
        set(NLOHMANN_JSON_VERSION "v3.12.0")
        message(STATUS "Using default nlohmann/json version: ${NLOHMANN_JSON_VERSION}")
    endif()
else()
    message(STATUS "Using nlohmann/json version from command line: ${NLOHMANN_JSON_VERSION}")
endif()

if(USE_SYSTEM_NLOHMANN_JSON)
    # Use system nlohmann/json
    message(STATUS "Using system nlohmann/json library")

    find_path(NLOHMANN_JSON_INCLUDE_DIR
        NAMES nlohmann/json.hpp
        PATHS
            /usr/include
            /usr/local/include
            /opt/nlohmann/include
    )

    if(NLOHMANN_JSON_INCLUDE_DIR)
        set(NLOHMANN_JSON_FOUND TRUE)
        set(NLOHMANN_JSON_INCLUDE_DIRS ${NLOHMANN_JSON_INCLUDE_DIR})
        message(STATUS "Found system nlohmann/json: ${NLOHMANN_JSON_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "nlohmann/json library not found. Please install nlohmann-json3-dev or similar package, or set USE_SYSTEM_NLOHMANN_JSON=OFF to download automatically.")
    endif()

else()
    # Download nlohmann/json using FetchContent
    message(STATUS "Downloading nlohmann/json library version ${NLOHMANN_JSON_VERSION}")

    # Configure nlohmann/json build options before FetchContent - disable all unnecessary components
    set(JSON_BuildTests OFF CACHE BOOL "Build nlohmann/json tests" FORCE)
    set(JSON_Install OFF CACHE BOOL "Install nlohmann/json" FORCE)
    set(JSON_MultipleHeaders OFF CACHE BOOL "Use non-amalgamated version of nlohmann/json" FORCE)

    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/${NLOHMANN_JSON_VERSION}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(nlohmann_json)

    # Set variables for downloaded version
    set(NLOHMANN_JSON_INCLUDE_DIRS ${nlohmann_json_SOURCE_DIR}/single_include)
    set(NLOHMANN_JSON_FOUND TRUE)

    # Make nlohmann_json target available with proper namespace for Inja
    if(TARGET nlohmann_json::nlohmann_json)
        # Target already exists with namespace
        set(NLOHMANN_JSON_TARGET nlohmann_json::nlohmann_json)
    elseif(TARGET nlohmann_json)
        # Create alias for compatibility
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
        set(NLOHMANN_JSON_TARGET nlohmann_json::nlohmann_json)
    else()
        # Create interface library if target doesn't exist
        add_library(nlohmann_json_interface INTERFACE)
        target_include_directories(nlohmann_json_interface INTERFACE ${NLOHMANN_JSON_INCLUDE_DIRS})
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json_interface)
        set(NLOHMANN_JSON_TARGET nlohmann_json::nlohmann_json)
    endif()

    message(STATUS "Using downloaded nlohmann/json library")
endif()

# ==============================================================================
# Inja Template Engine (Header-only) - After nlohmann/json
# ==============================================================================

# Option to control Inja usage
option(USE_SYSTEM_INJA "Use system-installed Inja template engine" OFF)

# Inja version to use when not using system version
if(NOT INJA_VERSION)
    if(DEFINED ENV{INJA_VERSION})
        set(INJA_VERSION $ENV{INJA_VERSION})
        message(STATUS "Using Inja version from environment variable: ${INJA_VERSION}")
    else()
        set(INJA_VERSION "3c58486e20d26467ae41161237a989a7b9960dd3")
        message(STATUS "Using default Inja version: ${INJA_VERSION}")
    endif()
else()
    message(STATUS "Using Inja version from command line: ${INJA_VERSION}")
endif()

if(USE_SYSTEM_INJA)
    # Use system Inja
    message(STATUS "Using system Inja template engine")

    find_path(INJA_INCLUDE_DIR
        NAMES inja.hpp
        PATHS
            /usr/include/inja
            /usr/local/include/inja
            /opt/inja/single_include/inja
    )

    if(INJA_INCLUDE_DIR)
        set(INJA_FOUND TRUE)
        set(INJA_INCLUDE_DIRS ${INJA_INCLUDE_DIR})
        message(STATUS "Found system Inja: ${INJA_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "Inja template engine not found. Please install inja or set USE_SYSTEM_INJA=OFF to download automatically.")
    endif()

else()
    # Download Inja using FetchContent
    message(STATUS "Downloading Inja template engine version ${INJA_VERSION}")

    # Configure Inja build options before FetchContent - disable all unnecessary components
    set(INJA_USE_EMBEDDED_JSON OFF CACHE BOOL "Use embedded JSON for Inja" FORCE)
    set(INJA_INSTALL OFF CACHE BOOL "Install Inja" FORCE)
    set(INJA_INSTALL_SINGLE_HEADER OFF CACHE BOOL "Install Inja single header" FORCE)
    set(INJA_EXPORT OFF CACHE BOOL "Export Inja" FORCE)
    set(INJA_BUILD_TESTS OFF CACHE BOOL "Build Inja tests" FORCE)

    # Set CMAKE_PREFIX_PATH so Inja can find our nlohmann_json
    if(NOT USE_SYSTEM_NLOHMANN_JSON)
        # Add our nlohmann_json to CMAKE_PREFIX_PATH for Inja to find
        list(APPEND CMAKE_PREFIX_PATH ${nlohmann_json_BINARY_DIR})
        list(APPEND CMAKE_PREFIX_PATH ${nlohmann_json_SOURCE_DIR})

        # Set nlohmann_json_DIR explicitly for Inja's find_package call
        if(EXISTS "${nlohmann_json_BINARY_DIR}/nlohmann_jsonConfig.cmake")
            set(nlohmann_json_DIR ${nlohmann_json_BINARY_DIR} CACHE PATH "Path to nlohmann_json config")
        elseif(EXISTS "${nlohmann_json_SOURCE_DIR}/cmake")
            set(nlohmann_json_DIR ${nlohmann_json_SOURCE_DIR}/cmake CACHE PATH "Path to nlohmann_json config")
        endif()
    endif()

    FetchContent_Declare(
        inja
        URL https://github.com/vowstar/inja/archive/${INJA_VERSION}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(inja)

    # Set variables for downloaded version
    set(INJA_INCLUDE_DIRS ${inja_SOURCE_DIR}/single_include)
    set(INJA_FOUND TRUE)
    message(STATUS "Using downloaded Inja template engine")
endif()

# ==============================================================================
# LibSystemRDL Core Library
# ==============================================================================

# Define source files for the library
set(SYSTEMRDL_LIB_SOURCES
    SystemRDLLexer.cpp
    SystemRDLParser.cpp
    SystemRDLBaseVisitor.cpp
    SystemRDLVisitor.cpp
    elaborator.cpp
    systemrdl_api.cpp
)

# Define public header files for the library
set(SYSTEMRDL_LIB_PUBLIC_HEADERS
    elaborator.h
    SystemRDLLexer.h
    SystemRDLParser.h
    SystemRDLBaseVisitor.h
    SystemRDLVisitor.h
    systemrdl_api.h
)

# Define private header files
set(SYSTEMRDL_LIB_PRIVATE_HEADERS
    cmdline_parser.h
)

# Create shared library if requested
if(SYSTEMRDL_BUILD_SHARED)
    add_library(systemrdl_shared SHARED ${SYSTEMRDL_LIB_SOURCES})

    # Set target properties
    set_target_properties(systemrdl_shared PROPERTIES
        OUTPUT_NAME systemrdl
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    # Include directories
    target_include_directories(systemrdl_shared
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/systemrdl>
        PRIVATE
            ${ANTLR4_INCLUDE_DIRS}
            ${INJA_INCLUDE_DIRS}
            ${NLOHMANN_JSON_INCLUDE_DIRS}
    )

    # Link libraries - handle ANTLR4 differently for downloaded vs system
    if(USE_SYSTEM_ANTLR4)
        target_link_libraries(systemrdl_shared
            PRIVATE
                ${ANTLR4_LIBRARIES}
        )
    else()
        # For downloaded ANTLR4, use the target we determined above
        add_dependencies(systemrdl_shared ${ANTLR4_TARGET})
        # Always use library file path to avoid CMake export issues with external FetchContent targets
        # Use TARGET_LINKER_FILE to get the correct library for linking (import lib on Windows, .a/.so on Unix)
        target_link_libraries(systemrdl_shared
            PRIVATE
                $<TARGET_LINKER_FILE:${ANTLR4_TARGET}>
        )
    endif()

    # Compile options
    target_compile_options(systemrdl_shared PRIVATE
        ${ANTLR4_CFLAGS_OTHER}
    )

    # Set compiler warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(systemrdl_shared PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
        )
    endif()

    # Create alias for consistent naming
    add_library(SystemRDL::systemrdl_shared ALIAS systemrdl_shared)
endif()

# Create static library if requested
if(SYSTEMRDL_BUILD_STATIC)
    add_library(systemrdl_static STATIC ${SYSTEMRDL_LIB_SOURCES})

    # Set target properties
    set_target_properties(systemrdl_static PROPERTIES
        OUTPUT_NAME systemrdl_static
        POSITION_INDEPENDENT_CODE ON
    )

    # Include directories
    target_include_directories(systemrdl_static
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/systemrdl>
        PRIVATE
            ${ANTLR4_INCLUDE_DIRS}
            ${INJA_INCLUDE_DIRS}
            ${NLOHMANN_JSON_INCLUDE_DIRS}
    )

    # Link libraries
    if(USE_SYSTEM_ANTLR4)
        target_link_libraries(systemrdl_static
            PRIVATE
                ${ANTLR4_LIBRARIES}
        )
    else()
        # For downloaded ANTLR4, use the library file directly to avoid export dependencies
        add_dependencies(systemrdl_static ${ANTLR4_TARGET})
        target_link_libraries(systemrdl_static
            PRIVATE
                $<TARGET_LINKER_FILE:${ANTLR4_TARGET}>
        )
    endif()

    # Compile options
    target_compile_options(systemrdl_static PRIVATE
        ${ANTLR4_CFLAGS_OTHER}
    )

    # Set compiler warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(systemrdl_static PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
        )
    endif()

    # Create alias for consistent naming
    add_library(SystemRDL::systemrdl_static ALIAS systemrdl_static)
endif()

# ==============================================================================
# Version and Git Information Compile Definitions
# ==============================================================================

# Option to include build timestamp (can break reproducible builds)
# Default to ON for main project, OFF for subprojects to avoid parent build issues
option(SYSTEMRDL_INCLUDE_BUILD_TIMESTAMP "Include build timestamp in version info" ${SYSTEMRDL_MAIN_PROJECT})

# Determine build timestamp for reproducible builds
set(SYSTEMRDL_BUILD_TIMESTAMP "unknown")
if(SYSTEMRDL_INCLUDE_BUILD_TIMESTAMP)
    # First priority: SOURCE_DATE_EPOCH for reproducible builds
    if(DEFINED ENV{SOURCE_DATE_EPOCH})
        # Convert SOURCE_DATE_EPOCH (Unix timestamp) to readable format
        execute_process(
            COMMAND date -u -d @$ENV{SOURCE_DATE_EPOCH} "+%Y-%m-%d %H:%M:%S UTC"
            OUTPUT_VARIABLE SYSTEMRDL_BUILD_TIMESTAMP
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(NOT SYSTEMRDL_BUILD_TIMESTAMP)
            # Fallback if date command fails
            set(SYSTEMRDL_BUILD_TIMESTAMP "$ENV{SOURCE_DATE_EPOCH}")
        endif()
        message(STATUS "Using SOURCE_DATE_EPOCH for build timestamp: ${SYSTEMRDL_BUILD_TIMESTAMP}")
    elseif(GIT_FOUND)
        # Second priority: Git commit timestamp (deterministic)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --format=%ci HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_TIMESTAMP
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(GIT_COMMIT_TIMESTAMP)
            set(SYSTEMRDL_BUILD_TIMESTAMP "${GIT_COMMIT_TIMESTAMP}")
            message(STATUS "Using git commit timestamp for build date: ${SYSTEMRDL_BUILD_TIMESTAMP}")
        else()
            set(SYSTEMRDL_BUILD_TIMESTAMP "unknown")
            message(STATUS "Git commit timestamp unavailable, using 'unknown'")
        endif()
    else()
        set(SYSTEMRDL_BUILD_TIMESTAMP "unknown")
        message(STATUS "Build timestamp disabled (no git), using 'unknown'")
    endif()
else()
    set(SYSTEMRDL_BUILD_TIMESTAMP "disabled")
    if(SYSTEMRDL_MAIN_PROJECT)
        message(STATUS "Build timestamp disabled by option")
    else()
        message(STATUS "Build timestamp disabled (subproject mode)")
    endif()
endif()

# Function to add version definitions to a target
function(add_version_definitions TARGET_NAME)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        SYSTEMRDL_GIT_COMMIT="${GIT_COMMIT_HASH}"
        SYSTEMRDL_GIT_BRANCH="${GIT_BRANCH_NAME}"
        SYSTEMRDL_BUILD_DATE="${SYSTEMRDL_BUILD_TIMESTAMP}"
    )
endfunction()

# Add version definitions to libraries
if(SYSTEMRDL_BUILD_SHARED)
    add_version_definitions(systemrdl_shared)
endif()

if(SYSTEMRDL_BUILD_STATIC)
    add_version_definitions(systemrdl_static)
endif()

# Create a generic alias that points to shared library if available, otherwise static
if(SYSTEMRDL_BUILD_SHARED)
    add_library(SystemRDL::systemrdl ALIAS systemrdl_shared)
    set(SYSTEMRDL_MAIN_TARGET systemrdl_shared)
elseif(SYSTEMRDL_BUILD_STATIC)
    add_library(SystemRDL::systemrdl ALIAS systemrdl_static)
    set(SYSTEMRDL_MAIN_TARGET systemrdl_static)
else()
    message(FATAL_ERROR "At least one of SYSTEMRDL_BUILD_SHARED or SYSTEMRDL_BUILD_STATIC must be ON")
endif()

# For tools that directly link to ANTLR4, prefer static linking to avoid mixing
# If we have both shared and static SystemRDL libraries built, prefer static for tools to avoid conflicts
if(SYSTEMRDL_BUILD_STATIC)
    set(SYSTEMRDL_TOOLS_TARGET systemrdl_static)
else()
    set(SYSTEMRDL_TOOLS_TARGET ${SYSTEMRDL_MAIN_TARGET})
endif()

# ==============================================================================
# Command Line Tools (Optional)
# ==============================================================================

if(SYSTEMRDL_BUILD_TOOLS)
# Create AST printer executable
add_executable(systemrdl_parser
    parser_main.cpp
)

# Create Elaborator executable
add_executable(systemrdl_elaborator
    elaborator_main.cpp
)

# Create CSV to RDL converter executable
add_executable(systemrdl_csv2rdl
    csv2rdl_main.cpp
)

# Create template renderer executable
add_executable(systemrdl_render
    render_main.cpp
)

# Create example application
add_executable(example
    example/example.cpp
)

    # Link tools to the library
    target_link_libraries(systemrdl_parser PRIVATE ${SYSTEMRDL_TOOLS_TARGET})
    target_link_libraries(systemrdl_elaborator PRIVATE ${SYSTEMRDL_TOOLS_TARGET})
    target_link_libraries(systemrdl_csv2rdl PRIVATE ${SYSTEMRDL_TOOLS_TARGET})
    target_link_libraries(systemrdl_render PRIVATE ${SYSTEMRDL_TOOLS_TARGET})
    target_link_libraries(example PRIVATE ${SYSTEMRDL_MAIN_TARGET})

    # Tools also need direct access to ANTLR4 since they use ANTLR4 classes directly
    if(USE_SYSTEM_ANTLR4)
        target_link_libraries(systemrdl_parser PRIVATE ${ANTLR4_LIBRARIES})
        target_link_libraries(systemrdl_elaborator PRIVATE ${ANTLR4_LIBRARIES})
        target_link_libraries(systemrdl_csv2rdl PRIVATE ${ANTLR4_LIBRARIES})
        target_link_libraries(systemrdl_render PRIVATE ${ANTLR4_LIBRARIES})
    else()
        # For downloaded ANTLR4, use the same target as determined for the platform
        # Don't mix static and shared - use only the target we configured
        target_link_libraries(systemrdl_parser PRIVATE $<TARGET_LINKER_FILE:${ANTLR4_TARGET}>)
        target_link_libraries(systemrdl_elaborator PRIVATE $<TARGET_LINKER_FILE:${ANTLR4_TARGET}>)
        target_link_libraries(systemrdl_csv2rdl PRIVATE $<TARGET_LINKER_FILE:${ANTLR4_TARGET}>)
        target_link_libraries(systemrdl_render PRIVATE $<TARGET_LINKER_FILE:${ANTLR4_TARGET}>)
        add_dependencies(systemrdl_parser ${ANTLR4_TARGET})
        add_dependencies(systemrdl_elaborator ${ANTLR4_TARGET})
        add_dependencies(systemrdl_csv2rdl ${ANTLR4_TARGET})
        add_dependencies(systemrdl_render ${ANTLR4_TARGET})
    endif()

    # Add ANTLR4 include directories for tools that need generated headers
target_include_directories(systemrdl_parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANTLR4_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
)
target_include_directories(systemrdl_elaborator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANTLR4_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
)
target_include_directories(systemrdl_render PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANTLR4_INCLUDE_DIRS}
    ${INJA_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
)
target_include_directories(example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

    # CSV converter now needs the full library includes since it uses the API
target_include_directories(systemrdl_csv2rdl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANTLR4_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
)

    # Set compiler warnings for tools
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(systemrdl_parser PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
        )
        target_compile_options(systemrdl_elaborator PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
        )
        target_compile_options(systemrdl_render PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
        )
        target_compile_options(systemrdl_csv2rdl PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
        )
        target_compile_options(example PRIVATE
            -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
        )
    endif()

    # Add version definitions to tools
    add_version_definitions(systemrdl_parser)
    add_version_definitions(systemrdl_elaborator)
    add_version_definitions(systemrdl_render)
    add_version_definitions(systemrdl_csv2rdl)
    add_version_definitions(example)
endif()

# ==============================================================================
# ANTLR4 Code Generation Targets
# ==============================================================================

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

# ==============================================================================
# Installation
# ==============================================================================

# Only install when SystemRDL is the main project, not when used as subproject
if(SYSTEMRDL_MAIN_PROJECT)

# Install library targets
if(SYSTEMRDL_BUILD_SHARED)
    install(TARGETS systemrdl_shared
        EXPORT SystemRDLTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(SYSTEMRDL_BUILD_STATIC)
    install(TARGETS systemrdl_static
        EXPORT SystemRDLTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
endif()

# Install tools if requested
if(SYSTEMRDL_BUILD_TOOLS)
    install(TARGETS systemrdl_parser systemrdl_elaborator systemrdl_csv2rdl systemrdl_render example
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install public headers
install(FILES ${SYSTEMRDL_LIB_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/systemrdl
)

# Install CMake config files
install(EXPORT SystemRDLTargets
    FILE SystemRDLTargets.cmake
    NAMESPACE SystemRDL::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SystemRDL
)

# Create and install package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SystemRDLConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SystemRDLConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SystemRDL
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SystemRDLConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SystemRDLConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SystemRDLConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SystemRDL
)

# Install pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/systemrdl.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/systemrdl.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/systemrdl.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

endif() # SYSTEMRDL_MAIN_PROJECT

# ==============================================================================
# Tests (Optional)
# ==============================================================================

if(SYSTEMRDL_BUILD_TESTS)
# Find all RDL test files
file(GLOB RDL_TEST_FILES "${CMAKE_SOURCE_DIR}/test/*.rdl")

    # Create tests for parser (only if tools are built)
    if(SYSTEMRDL_BUILD_TOOLS)
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

    # Check if this is an expected failure test - support both methods:
    # 1. New method: filename contains "_fail" suffix
    # 2. Legacy method: file content contains "EXPECT_ELABORATION_FAILURE"
    string(FIND "${test_name}" "_fail" is_failure_test_by_name)

    set(expect_failure FALSE)
    if(is_failure_test_by_name GREATER -1)
        # New naming convention: test_xxx_fail.rdl
        set(expect_failure TRUE)
    else()
        # Legacy method: check file content for EXPECT_ELABORATION_FAILURE
        file(READ ${rdl_file} file_content)
        string(FIND "${file_content}" "EXPECT_ELABORATION_FAILURE" expect_failure_pos)
        if(expect_failure_pos GREATER -1)
            set(expect_failure TRUE)
        endif()
    endif()

    add_test(
        NAME "elaborator_${test_name}"
        COMMAND systemrdl_elaborator ${rdl_file}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Set test properties
    if(expect_failure)
        # This test is expected to fail (elaborator should return non-zero exit code)
        set_tests_properties("elaborator_${test_name}" PROPERTIES
            LABELS "elaborator;expected_failure"
            WILL_FAIL TRUE
        )
    else()
        # This test is expected to succeed
        set_tests_properties("elaborator_${test_name}" PROPERTIES
            LABELS "elaborator"
        )
    endif()
endforeach()

# Find all CSV test files
file(GLOB CSV_TEST_FILES "${CMAKE_SOURCE_DIR}/test/test_csv_*.csv")

# Create tests for CSV converter
foreach(csv_file ${CSV_TEST_FILES})
    get_filename_component(test_name ${csv_file} NAME_WE)

    # Check if this is an expected failure test
    string(FIND "${test_name}" "_fail" is_failure_test)

    add_test(
        NAME "csv2rdl_${test_name}"
        COMMAND systemrdl_csv2rdl ${csv_file} -o ${test_name}.rdl
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    if(is_failure_test GREATER -1)
        # This test is expected to fail (CSV2RDL should return non-zero exit code)
        set_tests_properties("csv2rdl_${test_name}" PROPERTIES
            LABELS "csv2rdl;expected_failure"
            WILL_FAIL TRUE
        )
    else()
        # This test is expected to succeed
        set_tests_properties("csv2rdl_${test_name}" PROPERTIES
            LABELS "csv2rdl"
        )
    endif()
endforeach()
endif()

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

        # Skip failure tests for JSON output since they are expected to fail elaboration
        string(FIND "${test_name}" "_fail" is_failure_test_by_name)

        set(is_failure_test FALSE)
        if(is_failure_test_by_name GREATER -1)
            # New naming convention: test_xxx_fail.rdl
            set(is_failure_test TRUE)
        else()
            # Legacy method: check file content for EXPECT_ELABORATION_FAILURE
            file(READ ${rdl_file} file_content)
            string(FIND "${file_content}" "EXPECT_ELABORATION_FAILURE" expect_failure_pos)
            if(expect_failure_pos GREATER -1)
                set(is_failure_test TRUE)
            endif()
        endif()

        # Only create AST tests for success tests
        if(NOT is_failure_test)
            # AST test using unified Python script (replaces bash script)
            add_test(
                NAME "ast_test_${test_name}"
                COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/ast_output_validator.py
                        --test
                        --parser ${CMAKE_BINARY_DIR}/systemrdl_parser
                        --elaborator ${CMAKE_BINARY_DIR}/systemrdl_elaborator
                        --rdl ${rdl_file}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )
            set_tests_properties("ast_test_${test_name}" PROPERTIES
                LABELS "ast;test"
                DEPENDS "systemrdl_parser;systemrdl_elaborator"
            )

            # JSON test using unified Python script for simplified JSON
            add_test(
                NAME "json_test_${test_name}"
                COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/json_output_validator.py
                        --test
                        --elaborator ${CMAKE_BINARY_DIR}/systemrdl_elaborator
                        --rdl ${rdl_file}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )
            set_tests_properties("json_test_${test_name}" PROPERTIES
                LABELS "json;test"
                DEPENDS "systemrdl_elaborator"
            )
        endif()
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

    # Template Rendering Validation Test - validates template rendering with various templates
    add_test(
        NAME "template_render_validation"
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/script/template_render_validator.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties("template_render_validation" PROPERTIES
        LABELS "template;validation;render"
        TIMEOUT 180
        DEPENDS "systemrdl_render"
    )

    # Create test groups for convenience
    add_custom_target(test-ast
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "ast"
        DEPENDS systemrdl_parser systemrdl_elaborator
        COMMENT "Running AST output tests"
    )

    add_custom_target(test-json
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "json"
        DEPENDS systemrdl_elaborator
        COMMENT "Running simplified JSON output tests"
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

    add_custom_target(test-template
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "template"
        DEPENDS systemrdl_render
        COMMENT "Running template rendering tests"
    )

    # Custom target to run example tests (if example is built)
    if(TARGET example)
        add_custom_target(test-example
            COMMAND echo "Running SystemRDL example test..."
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR} $<TARGET_FILE:example>
            DEPENDS example
            COMMENT "Running SystemRDL example and API demonstration..."
        )
    else()
        add_custom_target(test-example
            COMMAND echo "[WARN] Example not built - skipping example test"
            COMMAND echo "   Enable with: cmake -DSYSTEMRDL_BUILD_TOOLS=ON"
            COMMENT "Example test skipped (example not built)"
        )
    endif()

    # Custom target to run fast tests (AST + JSON + semantic + CSV2RDL + template + example)
    add_custom_target(test-fast
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L ast
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L json
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L semantic
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L csv2rdl
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L template
        COMMAND echo "Running example integration test..."
        COMMAND ${CMAKE_COMMAND} --build . --target test-example
        DEPENDS systemrdl_parser systemrdl_elaborator systemrdl_csv2rdl systemrdl_render
        COMMENT "Running fast tests (AST + JSON + semantic + CSV2RDL + template + example validation)"
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
endif()

# ==============================================================================
# Code Quality and Formatting Targets (Only for main project)
# ==============================================================================

# Only create code quality targets when SystemRDL is the main project
# to avoid target name conflicts when used as a subproject
if(SYSTEMRDL_MAIN_PROJECT)

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
    "${CMAKE_SOURCE_DIR}/render_main.cpp"
    "${CMAKE_SOURCE_DIR}/systemrdl_api.cpp"
    "${CMAKE_SOURCE_DIR}/example/*.cpp"
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
    "${CMAKE_SOURCE_DIR}/doc/*.md"
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
            --dry-run
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
        COMMAND echo "[ERR] clang-format not found! Please install clang-format to check C++ formatting."
        COMMAND echo "   Ubuntu/Debian: sudo apt-get install clang-format"
        COMMAND echo "   Gentoo: sudo emerge clang"
        COMMAND false
    )

    add_custom_target(format-diff
        COMMAND echo "[ERR] clang-format not found! Please install clang-format to show formatting differences."
        COMMAND false
    )

    add_custom_target(format
        COMMAND echo "[ERR] clang-format not found! Please install clang-format to auto-format C++ code."
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
        COMMAND echo "Running cppcheck static analysis verbose..."
        COMMAND ${CPPCHECK_EXECUTABLE} --version
        COMMAND ${CPPCHECK_EXECUTABLE}
            --enable=warning,style,performance
            --suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppressions
            --inline-suppr
            --verbose
            ${CPP_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis verbose"
    )
else()
    # Fallback targets when cppcheck is not available
    add_custom_target(cppcheck
        COMMAND echo "[ERR] cppcheck not found! Please install cppcheck for static analysis."
        COMMAND echo "   Ubuntu/Debian: sudo apt-get install cppcheck"
        COMMAND echo "   Gentoo: sudo emerge dev-util/cppcheck"
        COMMAND false
    )

    add_custom_target(cppcheck-verbose
        COMMAND echo "[ERR] cppcheck not found! Please install cppcheck for static analysis."
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
        COMMAND echo "[ERR] Python not found! Please install Python to check Python code quality."
        COMMAND false
    )

    add_custom_target(python-format
        COMMAND echo "[ERR] Python not found! Please install Python to format Python code."
        COMMAND false
    )

    add_custom_target(python-lint
        COMMAND echo "[ERR] Python not found! Please install Python to lint Python code."
        COMMAND false
    )

    add_custom_target(python-quality
        COMMAND echo "[ERR] Python not found! Please install Python to check Python code quality."
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
        COMMAND echo "Available PyMarkdown rules:"
        COMMAND ${PYTHON_EXECUTABLE} -m pymarkdown plugins list
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Show available PyMarkdown rules"
    )
else()
    # Fallback targets when PyMarkdown is not available
    add_custom_target(markdown-lint
        COMMAND echo "[ERR] PyMarkdown not found! Please install pymarkdown to lint Markdown files."
        COMMAND echo "   pip install pymarkdown"
        COMMAND false
    )

    add_custom_target(markdown-fix
        COMMAND echo "[ERR] PyMarkdown not found! Please install pymarkdown to fix Markdown files."
        COMMAND false
    )

    add_custom_target(markdown-rules
        COMMAND echo "[ERR] PyMarkdown not found! Please install pymarkdown to show available rules."
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
    COMMAND echo "[INFO] Available Code Quality Targets:"
    COMMAND echo "=================================="
    COMMAND echo ""
    COMMAND echo "[CHECK] Code Checking:"
    COMMAND echo "  quality-check      - Run all quality checks"
    COMMAND echo "  format-check       - Check C++ code formatting \\(includes example/\\)"
    COMMAND echo "  format-diff        - Show C++ formatting differences"
    COMMAND echo "  cppcheck           - Run C++ static analysis \\(includes example/\\)"
    COMMAND echo "  cppcheck-verbose   - Run C++ static analysis verbose"
    COMMAND echo "  python-quality     - Run Python quality checks"
    COMMAND echo "  python-format-check - Check Python code formatting"
    COMMAND echo "  python-lint        - Run Python linting"
    COMMAND echo "  markdown-lint      - Lint Markdown files"
    COMMAND echo "  markdown-rules     - Show available markdown rules"
    COMMAND echo ""
    COMMAND echo "🧪 Testing:"
    COMMAND echo "  test-fast          - Quick tests \\(JSON + semantic + CSV2RDL + example\\)"
    COMMAND echo "  test-example       - Run example application \\(library integration test\\)"
    COMMAND echo "  test-parser        - Run parser tests only"
    COMMAND echo "  test-elaborator    - Run elaborator tests only"
    COMMAND echo "  run-tests          - Run all tests"
    COMMAND echo ""
    COMMAND echo "[FIX] Code Fixing:"
    COMMAND echo "  quality-format     - Auto-format all code"
    COMMAND echo "  format             - Auto-format C++ code \\(includes example/\\)"
    COMMAND echo "  python-format      - Auto-format Python code"
    COMMAND echo "  markdown-fix       - Auto-fix Markdown issues"
    COMMAND echo "  fix-all            - Fix all auto-fixable issues"
    COMMAND echo ""
    COMMAND echo "[TARGETS] Combined Targets:"
    COMMAND echo "  pre-commit         - Run pre-commit checks \\(quality + fast tests\\)"
    COMMAND echo "  quality-all        - Run complete quality analysis"
    COMMAND echo ""
    COMMAND echo "[EXAMPLES] Examples:"
    COMMAND echo "  make quality-check    # Check code quality \\(includes example/\\)"
    COMMAND echo "  make format           # Auto-format C++ code"
    COMMAND echo "  make test-example     # Test library integration"
    COMMAND echo "  make markdown-lint    # Check Markdown files"
    COMMAND echo "  make pre-commit       # Run what CI will check"
    COMMAND echo ""
    COMMENT "Showing code quality help"
)

endif() # SYSTEMRDL_MAIN_PROJECT - Code Quality Targets

# ==============================================================================
# Build Summary
# ==============================================================================

# Print configuration summary only when SystemRDL is the main project
if(SYSTEMRDL_MAIN_PROJECT)
    # Print configuration summary
    message(STATUS "")
    message(STATUS "SystemRDL-Toolkit Configuration Summary:")
    message(STATUS "  Version: ${PROJECT_VERSION}")
    message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
    message(STATUS "")
    message(STATUS "Build Options:")
    message(STATUS "  Build shared library: ${SYSTEMRDL_BUILD_SHARED}")
    message(STATUS "  Build static library: ${SYSTEMRDL_BUILD_STATIC}")
    message(STATUS "  Build command-line tools: ${SYSTEMRDL_BUILD_TOOLS}")
    message(STATUS "  Build tests: ${SYSTEMRDL_BUILD_TESTS}")
    message(STATUS "")
    message(STATUS "Dependencies:")
    message(STATUS "  ANTLR4 version: ${ANTLR4_VERSION}")
    message(STATUS "  Use system ANTLR4: ${USE_SYSTEM_ANTLR4}")
    message(STATUS "  Inja version: ${INJA_VERSION}")
    message(STATUS "  Use system Inja: ${USE_SYSTEM_INJA}")
    message(STATUS "  Python3 found: ${Python3_FOUND}")
    if(Python3_FOUND)
        message(STATUS "  Python executable: ${PYTHON_EXECUTABLE}")
    endif()
    message(STATUS "")
    message(STATUS "Code Quality:")
    message(STATUS "  Code quality targets available: YES (main project)")
    message(STATUS "  Run 'make quality-help' for available targets")
    message(STATUS "")
    message(STATUS "Install Locations:")
    message(STATUS "  Prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  Libraries: ${CMAKE_INSTALL_FULL_LIBDIR}")
    message(STATUS "  Headers: ${CMAKE_INSTALL_FULL_INCLUDEDIR}/systemrdl")
    message(STATUS "  CMake config: ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/SystemRDL")
    if(SYSTEMRDL_BUILD_TOOLS)
        message(STATUS "  Executables: ${CMAKE_INSTALL_FULL_BINDIR}")
    endif()
    message(STATUS "")
else()
    # Brief summary for subproject builds
    message(STATUS "SystemRDL-Toolkit ${PROJECT_VERSION} configured as subproject")
    set(LIBRARY_TYPES "")
    if(SYSTEMRDL_BUILD_SHARED)
        set(LIBRARY_TYPES "${LIBRARY_TYPES}shared ")
    endif()
    if(SYSTEMRDL_BUILD_STATIC)
        set(LIBRARY_TYPES "${LIBRARY_TYPES}static")
    endif()
    message(STATUS "  Libraries: ${LIBRARY_TYPES}")
    if(SYSTEMRDL_BUILD_TOOLS)
        message(STATUS "  Tools enabled")
    endif()
    if(SYSTEMRDL_BUILD_TESTS)
        message(STATUS "  Tests enabled")
    endif()
    message(STATUS "  Code quality targets: DISABLED (subproject - avoids target conflicts)")
endif()
