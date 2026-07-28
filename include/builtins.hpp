#pragma once

#include "executor.hpp"

#include <string>
#include <vector>
#include <map>
#include <deque>

namespace tinyshell {

class Builtins {
public:
    Builtins();
    ~Builtins() = default;

    bool isBuiltin(const std::string& command);
    bool execute(const std::vector<std::string>& args, Executor& executor);

    std::string currentDirectory() const;
    int lastExitStatus() const;
    void setLastExitStatus(int status);

    std::string getVariable(const std::string& name) const;
    void setVariable(const std::string& name, const std::string& value);
    void unsetVariable(const std::string& name);
    const std::map<std::string, std::string>& variables() const;

    void addHistory(const std::string& line);
    const std::deque<std::string>& history() const;
    void clearHistory();

private:
    bool cd(const std::vector<std::string>& args);
    bool pwd();
    bool echo(const std::vector<std::string>& args);
    bool help();
    bool exit(const std::vector<std::string>& args);
    bool set(const std::vector<std::string>& args);
    bool unset(const std::vector<std::string>& args);
    bool env(const std::vector<std::string>& args);
    bool export_cmd(const std::vector<std::string>& args);
    bool jobs(const std::vector<std::string>& args, Executor& executor);
    bool fg(const std::vector<std::string>& args, Executor& executor);
    bool bg(const std::vector<std::string>& args, Executor& executor);
    bool history_cmd(const std::vector<std::string>& args);

    std::string currentDirectory_;
    int lastExitStatus_{0};
    std::map<std::string, std::string> variables_;
    std::deque<std::string> history_;
    static constexpr size_t kMaxHistory = 100;
};

} // namespace tinyshell
