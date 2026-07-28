#include "shell.hpp"
#include "parser.hpp"
#include "executor.hpp"
#include "builtins.hpp"
#include "signals.hpp"

#include <iostream>
#include <string>
#include <limits>
#include <unistd.h>

namespace tinyshell {

Shell::Shell()
    : parser_(std::make_unique<Parser>()),
      executor_(std::make_unique<Executor>()),
      builtins_(std::make_unique<Builtins>()),
      inputStream_(&std::cin),
      outputStream_(&std::cout),
      running_(true),
      currentDirectory_(builtins_->currentDirectory()) {
    Signals::setup();
}

Shell::~Shell() {
    Signals::reset();
}

void Shell::run() {
    std::string line;

    while (running_) {
        displayPrompt();

        if (!std::getline(*inputStream_, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        builtins_->addHistory(line);
        processCommand(line);
    }

    *outputStream_ << "exit\n";
}

void Shell::displayPrompt() {
    size_t pos = currentDirectory_.find_last_of('/');
    std::string dirName = (pos != std::string::npos)
        ? currentDirectory_.substr(pos + 1)
        : currentDirectory_;

    *outputStream_ << dirName << "$ ";
    outputStream_->flush();
}

std::string Shell::expandVariables(const std::string& input) {
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '$' && i + 1 < input.size()) {
            ++i;

            if (input[i] == '{') {
                ++i;
                size_t end = input.find('}', i);
                if (end != std::string::npos) {
                    std::string var_name = input.substr(i, end - i);
                    result += builtins_->getVariable(var_name);
                    i = end;
                } else {
                    result += "${";
                }
            } else if (input[i] == '?') {
                result += std::to_string(builtins_->lastExitStatus());
            } else if (input[i] == '$') {
                result += std::to_string(getpid());
            } else if (std::isalpha(input[i]) || input[i] == '_') {
                size_t start = i;
                while (i < input.size() && (std::isalnum(input[i]) || input[i] == '_')) {
                    ++i;
                }
                std::string var_name = input.substr(start, i - start);
                result += builtins_->getVariable(var_name);
                --i;
            } else {
                result += '$';
                result += input[i];
            }
        } else if (input[i] == '\\' && i + 1 < input.size() && input[i + 1] == '$') {
            ++i;
            result += '$';
        } else {
            result += input[i];
        }
    }

    return result;
}

bool Shell::processCommand(const std::string& line) {
    try {
        std::string expanded = expandVariables(line);
        auto pipeline = parser_->parse(expanded);

        if (pipeline.commands.empty()) {
            return true;
        }

        if (pipeline.commands.size() == 1) {
            const auto& cmd = pipeline.commands[0];
            if (builtins_->isBuiltin(cmd.args[0])) {
                if (cmd.args[0] == "cd") {
                    bool success = builtins_->execute(cmd.args, *executor_);
                    currentDirectory_ = builtins_->currentDirectory();
                    builtins_->setLastExitStatus(success ? 0 : 1);
                    return success;
                }
                if (cmd.args[0] == "exit") {
                    running_ = false;
                    return true;
                }
                bool success = builtins_->execute(cmd.args, *executor_);
                builtins_->setLastExitStatus(success ? 0 : 1);
                return success;
            }
        }

        bool success = executor_->execute(pipeline);
        builtins_->setLastExitStatus(success ? 0 : 1);
        return success;

    } catch (const std::exception& e) {
        *outputStream_ << "Error: " << e.what() << '\n';
        builtins_->setLastExitStatus(1);
        return false;
    }
}

void Shell::setInputStream(std::istream& stream) {
    inputStream_ = &stream;
}

void Shell::setOutputStream(std::ostream& stream) {
    outputStream_ = &stream;
}

} // namespace tinyshell
