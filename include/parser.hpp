#pragma once

#include <string>
#include <vector>

namespace tinyshell {

struct Command {
    std::vector<std::string> args;
    std::string inputFile;
    std::string outputFile;
    bool appendOutput = false;
    bool background = false;
};

struct Pipeline {
    std::vector<Command> commands;
};

class Parser {
public:
    Parser() = default;
    ~Parser() = default;

    Pipeline parse(const std::string& line);
    std::vector<std::string> tokenize(const std::string& line);

private:
    Command parseCommand(const std::string& cmdStr);
    void handleRedirect(Command& cmd, const std::string& token);
};

} // namespace tinyshell
