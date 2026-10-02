cmake_minimum_required(VERSION 3.16)
project(SystemRDL_Example CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Consume the installed package the same way an outside project does, so this
# example fails when the package itself is broken. Install the library first:
#
#     cmake -B build && cmake --install build --prefix /usr/local
find_package(SystemRDL REQUIRED)

add_executable(example_app example.cpp)
target_link_libraries(example_app PRIVATE SystemRDL::systemrdl)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(example_app PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
