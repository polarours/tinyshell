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

set(LIB_SOURCES
    src/parser.cpp
    src/executor.cpp
    src/builtins.cpp
    src/signals.cpp
    src/shell.cpp
)

if(BUILD_TESTING)
    add_executable(tinyshell_tests
        tests/tinyshell_tests.cpp
        ${LIB_SOURCES}
    )
    target_include_directories(tinyshell_tests PRIVATE include)
    target_link_libraries(tinyshell_tests PRIVATE Threads::Threads)
    add_test(NAME tinyshell_tests COMMAND tinyshell_tests)

    add_executable(tinyshell_e2e_tests
        tests/e2e_session_tests.cpp
        ${LIB_SOURCES}
    )
    target_include_directories(tinyshell_e2e_tests PRIVATE include)
    target_link_libraries(tinyshell_e2e_tests PRIVATE Threads::Threads)
    add_test(NAME tinyshell_e2e_tests COMMAND tinyshell_e2e_tests)
    set_tests_properties(tinyshell_e2e_tests PROPERTIES
        ENVIRONMENT "TINYSHELL_BIN=$<TARGET_FILE:tinyshell>")
endif()

# Exercises are intentionally RED: they describe behaviour the student must
# implement. They are opt-in so the normal build and test suite stay green.
option(TINYSHELL_BUILD_EXERCISES "Build the intentionally failing exercises" OFF)
if(TINYSHELL_BUILD_EXERCISES)
    set(EXERCISES
        01_quote_aware_pipes
        02_backslash_escaping
        03_builtin_redirects
        04_pipeline_fd_array
        05_exact_exit_status
        06_single_quotes_literal
        07_redirect_failure_aborts
    )
    foreach(exercise IN LISTS EXERCISES)
        add_executable(exercise_${exercise}
            tests/exercises/${exercise}.cpp
            ${LIB_SOURCES}
        )
        target_include_directories(exercise_${exercise} PRIVATE include tests/exercises)
        target_link_libraries(exercise_${exercise} PRIVATE Threads::Threads)
        add_test(NAME exercise_${exercise} COMMAND exercise_${exercise})
        set_tests_properties(exercise_${exercise} PROPERTIES
            ENVIRONMENT "TINYSHELL_BIN=$<TARGET_FILE:tinyshell>")
        if(exercise STREQUAL "04_pipeline_fd_array")
            # This lab is expected to hang on the out-of-bounds fd array.
            set_tests_properties(exercise_${exercise} PROPERTIES TIMEOUT 8)
        endif()
    endforeach()
endif()
