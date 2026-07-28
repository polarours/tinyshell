#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tinyshell {

class Parser;
class Executor;
class Builtins;

class Shell {
public:
    Shell();
    ~Shell();

    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;
    Shell(Shell&&) = delete;
    Shell& operator=(Shell&&) = delete;

    void run();

    void setInputStream(std::istream& stream);
    void setOutputStream(std::ostream& stream);

private:
    void displayPrompt();
    bool processCommand(const std::string& line);
    std::string expandVariables(const std::string& input);

    std::unique_ptr<Parser> parser_;
    std::unique_ptr<Executor> executor_;
    std::unique_ptr<Builtins> builtins_;

    std::istream* inputStream_;
    std::ostream* outputStream_;

    bool running_;
    std::string currentDirectory_;
};

} // namespace tinyshell
