// Exercise 05: preserve the real exit status, not just success/failure.
// `sh -c 'exit 7'` must make `$?` equal 7, not 1.
#include "harness.hpp"

#include <string>

int main() {
    int failed = 0;
    std::string out = exercise::session(
        "sh -c 'exit 7'\necho $?\nexit\n");

    failed += exercise::expect(out.find("\n7") != std::string::npos ||
                                   out.find("$ 7") != std::string::npos,
                               "$? must report the exact exit status 7");

    return failed == 0 ? 0 : 1;
}
