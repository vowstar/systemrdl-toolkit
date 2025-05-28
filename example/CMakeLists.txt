cmake_minimum_required(VERSION 3.16)
project(SystemRDL-Example
    VERSION 1.0.0
    DESCRIPTION "Example of using SystemRDL library"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the SystemRDL library
find_package(SystemRDL REQUIRED)

# Create example executable that uses the library
add_executable(example_app example.cpp)

# Link against SystemRDL library
target_link_libraries(example_app PRIVATE SystemRDL::systemrdl)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(example_app PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
    )
endif()

# Optional: show information about the found library
message(STATUS "Using SystemRDL library:")
message(STATUS "  Version: ${SYSTEMRDL_VERSION}")
message(STATUS "  Include dirs: ${SYSTEMRDL_INCLUDE_DIRS}")
message(STATUS "  Libraries: ${SYSTEMRDL_LIBRARIES}")

# Optional: Install the example
install(TARGETS example_app
    RUNTIME DESTINATION bin
)