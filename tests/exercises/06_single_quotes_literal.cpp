// Exercise 06: single quotes must protect variables from expansion.
#include "harness.hpp"

#include <string>

int main() {
    int failed = 0;
    std::string out = exercise::session(
        "set NAME=student\necho '$NAME'\nexit\n");

    failed += exercise::expect(out.find("$NAME") != std::string::npos,
                               "single-quoted $NAME must stay literal");
    failed += exercise::expect(out.find("student") == std::string::npos,
                               "single quotes must suppress variable expansion");

    return failed == 0 ? 0 : 1;
}
