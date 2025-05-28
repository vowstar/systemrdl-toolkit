cmake_minimum_required(VERSION 3.16)
project(SystemRDL_Example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use already built SystemRDL library from parent build directory
message(STATUS "Using pre-built SystemRDL library from parent build")

# Set paths to the built library
set(SYSTEMRDL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(SYSTEMRDL_BUILD_DIR "${SYSTEMRDL_ROOT_DIR}/build")

# Check if library exists
if(NOT EXISTS "${SYSTEMRDL_BUILD_DIR}/libsystemrdl_static.a")
    message(FATAL_ERROR "SystemRDL library not found. Please build the parent project first:
    cd ${SYSTEMRDL_ROOT_DIR}
    mkdir -p build && cd build
    cmake .. && make")
endif()

# Find ANTLR4 (required by SystemRDL)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(ANTLR4 QUIET antlr4-runtime)
endif()

if(NOT ANTLR4_FOUND)
    # Try to find system ANTLR4
    find_path(ANTLR4_INCLUDE_DIR antlr4-runtime/antlr4-runtime.h
        PATHS /usr/include /usr/local/include)
    find_library(ANTLR4_LIBRARY antlr4-runtime
        PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64)

    if(ANTLR4_INCLUDE_DIR AND ANTLR4_LIBRARY)
        set(ANTLR4_INCLUDE_DIRS ${ANTLR4_INCLUDE_DIR})
        set(ANTLR4_LIBRARIES ${ANTLR4_LIBRARY})
        message(STATUS "Found system ANTLR4: ${ANTLR4_LIBRARY}")
    else()
        message(FATAL_ERROR "ANTLR4 not found. Please install antlr4-runtime development package.")
    endif()
endif()

# Create SystemRDL imported library
add_library(SystemRDL::systemrdl STATIC IMPORTED)
set_target_properties(SystemRDL::systemrdl PROPERTIES
    IMPORTED_LOCATION "${SYSTEMRDL_BUILD_DIR}/libsystemrdl_static.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SYSTEMRDL_ROOT_DIR};${ANTLR4_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${ANTLR4_LIBRARIES}"
)

# Create the example application
add_executable(example_app example.cpp)

# Link against SystemRDL library
target_link_libraries(example_app PRIVATE SystemRDL::systemrdl)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(example_app PRIVATE
        -Wall -Wextra -Wno-unused-parameter
    )
endif()

# Print configuration info
message(STATUS "SystemRDL Example Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  SystemRDL library: ${SYSTEMRDL_BUILD_DIR}/libsystemrdl_static.a")
message(STATUS "  ANTLR4 includes: ${ANTLR4_INCLUDE_DIRS}")
message(STATUS "  ANTLR4 libraries: ${ANTLR4_LIBRARIES}")
