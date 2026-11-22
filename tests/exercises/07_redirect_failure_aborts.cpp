// Exercise 07: a failed redirection must abort the command.
// handleRedirects() already reports failure, but its caller ignores it, so
// `echo` still writes to stdout when the target directory does not exist.
#include "harness.hpp"

#include <string>

int main() {
    int failed = 0;
    std::string out = exercise::session(
        "echo should-not-print > /no/such/directory/out\nexit\n");

    failed += exercise::expect(out.find("should-not-print") == std::string::npos,
                               "output must not escape a failed redirection");

    return failed == 0 ? 0 : 1;
}
