// Exercise 01: a pipe character inside quotes is data, not a separator.
#include "harness.hpp"

int main() {
    int failed = 0;
    tinyshell::Parser parser;

    auto pipeline = parser.parse("echo \"a|b\"");
    failed += exercise::expect(pipeline.commands.size() == 1,
                               "quoted '|' must not create a second command");
    if (pipeline.commands.size() == 1) {
        failed += exercise::expect(pipeline.commands[0].args.size() == 2,
                                   "expected echo plus one argument");
        if (pipeline.commands[0].args.size() == 2) {
            failed += exercise::expect(pipeline.commands[0].args[1] == "a|b",
                                       "the quoted pipe must survive as data");
        }
    }

    return failed == 0 ? 0 : 1;
}
