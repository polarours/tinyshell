#pragma once

#include "builtins.hpp"
#include "executor.hpp"
#include "parser.hpp"
#include "shell.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace exercise {

inline int expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return 1;
    }
    return 0;
}

namespace detail {

inline std::string quote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    return out + "'";
}

} // namespace detail

// Drive the real tinyshell binary with a scripted stdin session. Capturing
// the process stdout is essential: builtins write to std::cout and external
// commands inherit fd 1, so an ostringstream inside Shell would miss both.
inline std::string session(const std::string& script) {
    const char* binary = std::getenv("TINYSHELL_BIN");
    const char* tmp = std::getenv("TMPDIR");
    if (!binary) {
        return "TINYSHELL_BIN is not set";
    }

    std::string dir = tmp ? tmp : "/tmp";
    std::string path = dir + "/tinyshell_session_" + std::to_string(getpid());
    {
        std::ofstream file(path);
        file << script;
    }

    std::string command = std::string("timeout 5 ") + binary +
                          " < " + detail::quote(path) + " 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    std::string output;
    if (pipe) {
        char buffer[1024];
        size_t read;
        while ((read = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
            output.append(buffer, read);
        }
        pclose(pipe);
    }
    std::remove(path.c_str());
    return output;
}

} // namespace exercise
