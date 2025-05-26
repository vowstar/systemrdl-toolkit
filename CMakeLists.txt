cmake_minimum_required(VERSION 3.16)
project(SystemRDLParser)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
install(TARGETS systemrdl_parser
    RUNTIME DESTINATION bin
)

# Print configuration information
message(STATUS "ANTLR4 include dirs: ${ANTLR4_INCLUDE_DIRS}")
message(STATUS "ANTLR4 libraries: ${ANTLR4_LIBRARIES}")