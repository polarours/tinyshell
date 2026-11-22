// Exercise 03: builtins must honour redirections. Shell::processCommand
// dispatches a single builtin straight to Builtins::execute, which writes to
// std::cout and never sees cmd.outputFile.
#include "harness.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

int main() {
    const char* tmp = std::getenv("TMPDIR");
    std::string path = (tmp ? std::string(tmp) : std::string("/tmp")) +
                       "/tinyshell_exercise_builtin_redirect.txt";
    std::remove(path.c_str());

    std::string script = "echo captured > " + path + "\nexit\n";
    (void)exercise::session(script);

    std::ifstream file(path);
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    std::remove(path.c_str());

    int failed = exercise::expect(contents.find("captured") != std::string::npos,
                                  "a builtin must write into the redirected file");
    return failed == 0 ? 0 : 1;
}
