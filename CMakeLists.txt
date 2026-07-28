cmake_minimum_required(VERSION 3.14)
project(tinyshell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/shell.cpp
    src/parser.cpp
    src/executor.cpp
    src/builtins.cpp
    src/signals.cpp
)

# Create executable
add_executable(tinyshell ${SOURCES})

# Include directories
target_include_directories(tinyshell PRIVATE include)

# Find required libraries
find_package(Threads REQUIRED)
target_link_libraries(tinyshell PRIVATE Threads::Threads)

# Enable testing
enable_testing()

if(BUILD_TESTING)
    add_executable(tinyshell_tests
        tests/tinyshell_tests.cpp
        src/parser.cpp
        src/builtins.cpp
        src/executor.cpp
        src/signals.cpp
    )
    target_include_directories(tinyshell_tests PRIVATE include)
    target_link_libraries(tinyshell_tests PRIVATE Threads::Threads)
    add_test(NAME tinyshell_tests COMMAND tinyshell_tests)
endif()
