// Exercise 04: the fd array must hold two ints per pipe, not one.
// Executor::execute() sizes it as commands - 1 and then indexes pairs, so
// even `a | b` writes out of bounds and the session hangs or corrupts memory.
#include "harness.hpp"

#include <string>

int main() {
    int failed = 0;
    std::string out = exercise::session(
        "printf 'a\\nb\\n' | wc -l\nexit\n");

    failed += exercise::expect(out.find("2") != std::string::npos,
                               "pipeline must return the line count 2");
    failed += exercise::expect(out.find("Assertion") == std::string::npos,
                               "the fd array must not be indexed out of bounds");

    return failed == 0 ? 0 : 1;
}
