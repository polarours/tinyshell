// Exercise 02: backslash escapes a space outside quotes.
#include "harness.hpp"

int main() {
    int failed = 0;
    tinyshell::Parser parser;

    auto tokens = parser.tokenize("echo one\\ two");
    failed += exercise::expect(tokens.size() == 2, "expected exactly two tokens");
    if (tokens.size() == 2) {
        failed += exercise::expect(tokens[0] == "echo", "first token must be echo");
        failed += exercise::expect(tokens[1] == "one two",
                                   "the escaped space must belong to one token");
    }

    return failed == 0 ? 0 : 1;
}
