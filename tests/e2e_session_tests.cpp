// End-to-end tests: drive the real Shell with scripted sessions.
// These are characterization tests for behaviour that already works.
#include "exercises/harness.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace {

int test_echo() {
    std::string out = exercise::session("echo hello\nexit\n");
    return exercise::expect(out.find("hello") != std::string::npos,
                            "echo must reach stdout") +
           exercise::expect(out.find("exit\n") != std::string::npos,
                            "run() must close with an exit line");
}

int test_cd_changes_directory() {
    std::string out = exercise::session("cd /tmp\npwd\nexit\n");
    return exercise::expect(out.find("/tmp") != std::string::npos,
                            "cd must change the shell's working directory");
}

int test_variable_expansion() {
    std::string out = exercise::session(
        "set GREETING=hello\necho $GREETING\nexit\n");
    return exercise::expect(out.find("hello") != std::string::npos,
                            "set + $VAR must expand in the same session");
}

int test_redirect_to_file() {
    const char* tmp = std::getenv("TMPDIR");
    std::string path = (tmp ? std::string(tmp) : std::string("/tmp")) +
                       "/tinyshell_e2e_redirect.txt";
    std::remove(path.c_str());

    std::string script = "printf redirected > " + path + "\nexit\n";
    exercise::session(script);

    std::ifstream file(path);
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    std::remove(path.c_str());

    return exercise::expect(contents.find("redirected") != std::string::npos,
                            "redirected output must land in the file");
}

} // namespace

int main() {
    int failed = 0;
    failed += test_echo();
    failed += test_cd_changes_directory();
    failed += test_variable_expansion();
    failed += test_redirect_to_file();

    if (failed == 0) {
        std::cout << "all end-to-end sessions passed\n";
    }
    return failed == 0 ? 0 : 1;
}
