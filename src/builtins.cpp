#include "builtins.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <algorithm>
#include <cstdlib>
#include <csignal>

namespace tinyshell {

Builtins::Builtins() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        currentDirectory_ = cwd;
    }

    variables_["?"] = "0";
}

bool Builtins::isBuiltin(const std::string& command) {
    static const std::vector<std::string> builtins = {
        "cd", "exit", "help", "pwd", "echo",
        "set", "unset", "env", "export",
        "jobs", "fg", "bg", "history"
    };

    return std::find(builtins.begin(), builtins.end(), command) != builtins.end();
}

bool Builtins::execute(const std::vector<std::string>& args, Executor& executor) {
    if (args.empty()) {
        return false;
    }

    const std::string& command = args[0];

    if (command == "cd") {
        return cd(args);
    } else if (command == "pwd") {
        return pwd();
    } else if (command == "echo") {
        return echo(args);
    } else if (command == "help") {
        return help();
    } else if (command == "exit") {
        return exit(args);
    } else if (command == "set") {
        return set(args);
    } else if (command == "unset") {
        return unset(args);
    } else if (command == "env") {
        return env(args);
    } else if (command == "export") {
        return export_cmd(args);
    } else if (command == "jobs") {
        return jobs(args, executor);
    } else if (command == "fg") {
        return fg(args, executor);
    } else if (command == "bg") {
        return bg(args, executor);
    } else if (command == "history") {
        return history_cmd(args);
    }

    return false;
}

std::string Builtins::currentDirectory() const {
    return currentDirectory_;
}

int Builtins::lastExitStatus() const {
    return lastExitStatus_;
}

void Builtins::setLastExitStatus(int status) {
    lastExitStatus_ = status;
    variables_["?"] = std::to_string(status);
}

std::string Builtins::getVariable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return it->second;
    }

    const char* env_val = getenv(name.c_str());
    if (env_val) {
        return env_val;
    }

    return "";
}

void Builtins::setVariable(const std::string& name, const std::string& value) {
    variables_[name] = value;
}

void Builtins::unsetVariable(const std::string& name) {
    variables_.erase(name);
}

const std::map<std::string, std::string>& Builtins::variables() const {
    return variables_;
}

void Builtins::addHistory(const std::string& line) {
    if (line.empty()) {
        return;
    }

    if (!history_.empty() && history_.back() == line) {
        return;
    }

    history_.push_back(line);
    if (history_.size() > kMaxHistory) {
        history_.pop_front();
    }
}

const std::deque<std::string>& Builtins::history() const {
    return history_;
}

void Builtins::clearHistory() {
    history_.clear();
}

bool Builtins::cd(const std::vector<std::string>& args) {
    std::string target;

    if (args.size() < 2 || args[1] == "~") {
        const char* home = getenv("HOME");
        if (!home) {
            std::cerr << "cd: HOME not set\n";
            return false;
        }
        target = home;
    } else if (args[1] == "-") {
        target = getVariable("OLDPWD");
        if (target.empty()) {
            std::cerr << "cd: OLDPWD not set\n";
            return false;
        }
        std::cout << target << '\n';
    } else {
        target = args[1];
    }

    char old_cwd[1024];
    if (getcwd(old_cwd, sizeof(old_cwd)) != nullptr) {
        setVariable("OLDPWD", old_cwd);
    }

    if (chdir(target.c_str()) == -1) {
        perror("cd");
        return false;
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        currentDirectory_ = cwd;
        setVariable("PWD", cwd);
    }

    return true;
}

bool Builtins::pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << cwd << '\n';
        return true;
    }

    perror("pwd");
    return false;
}

bool Builtins::echo(const std::vector<std::string>& args) {
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) {
            std::cout << ' ';
        }
        std::cout << args[i];
    }
    std::cout << '\n';
    return true;
}

bool Builtins::help() {
    std::cout << "tinyshell - A minimal Unix shell\n\n";
    std::cout << "Built-in commands:\n";
    std::cout << "  cd [dir]       Change directory (default: HOME)\n";
    std::cout << "  pwd            Print working directory\n";
    std::cout << "  echo [args]    Print arguments\n";
    std::cout << "  help           Show this help message\n";
    std::cout << "  exit [code]    Exit the shell\n";
    std::cout << "  set [name=val] Set or list shell variables\n";
    std::cout << "  unset name     Remove a shell variable\n";
    std::cout << "  env            Print environment variables\n";
    std::cout << "  export name    Export variable to environment\n";
    std::cout << "  jobs           List background jobs\n";
    std::cout << "  fg [job]       Bring job to foreground\n";
    std::cout << "  bg [job]       Resume job in background\n";
    std::cout << "  history        Show command history\n\n";
    std::cout << "Features:\n";
    std::cout << "  - External command execution\n";
    std::cout << "  - I/O redirection: <, >, >>\n";
    std::cout << "  - Pipelines: cmd1 | cmd2 | cmd3\n";
    std::cout << "  - Background processes: command &\n";
    std::cout << "  - Quote handling: \"double\" and 'single'\n";
    std::cout << "  - Shell variables and $ expansion\n";
    std::cout << "  - Exit status tracking ($?)\n";
    std::cout << "  - Command history\n";
    return true;
}

bool Builtins::exit(const std::vector<std::string>& args) {
    int code = lastExitStatus_;
    if (args.size() > 1) {
        try {
            code = std::stoi(args[1]);
        } catch (...) {
            code = 1;
        }
    }

    std::exit(code);
    return true;
}

bool Builtins::set(const std::vector<std::string>& args) {
    if (args.size() == 1) {
        for (const auto& [name, value] : variables_) {
            std::cout << name << "=" << value << '\n';
        }
        return true;
    }

    for (size_t i = 1; i < args.size(); ++i) {
        const auto& arg = args[i];
        size_t eq_pos = arg.find('=');
        if (eq_pos != std::string::npos) {
            std::string name = arg.substr(0, eq_pos);
            std::string value = arg.substr(eq_pos + 1);
            setVariable(name, value);
        } else {
            setVariable(arg, "");
        }
    }
    return true;
}

bool Builtins::unset(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "unset: missing operand\n";
        return false;
    }

    for (size_t i = 1; i < args.size(); ++i) {
        unsetVariable(args[i]);
    }
    return true;
}

bool Builtins::env(const std::vector<std::string>& /*args*/) {
    for (const auto& [name, value] : variables_) {
        std::cout << name << "=" << value << '\n';
    }
    return true;
}

bool Builtins::export_cmd(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "export: missing operand\n";
        return false;
    }

    for (size_t i = 1; i < args.size(); ++i) {
        const auto& name = args[i];
        std::string value = getVariable(name);
        setenv(name.c_str(), value.c_str(), 1);
    }
    return true;
}

bool Builtins::jobs(const std::vector<std::string>& /*args*/, Executor& executor) {
    executor.updateJobStatus();

    const auto& jobs = executor.jobs();
    if (jobs.empty()) {
        return true;
    }

    for (const auto& [id, job] : jobs) {
        std::string state;
        switch (job.state) {
            case JobState::running: state = "Running"; break;
            case JobState::stopped: state = "Stopped"; break;
            case JobState::done: state = "Done"; break;
        }
        std::cout << "[" << job.id << "] " << state << "\t" << job.command << '\n';
    }

    return true;
}

bool Builtins::fg(const std::vector<std::string>& args, Executor& executor) {
    if (args.size() < 2) {
        std::cerr << "fg: job ID required\n";
        return false;
    }

    int jobId;
    try {
        jobId = std::stoi(args[1]);
    } catch (...) {
        std::cerr << "fg: invalid job ID: " << args[1] << '\n';
        return false;
    }

    Job* job = executor.findJob(jobId);
    if (!job) {
        std::cerr << "fg: no such job: " << jobId << '\n';
        return false;
    }

    if (job->state == JobState::stopped) {
        kill(job->pid, SIGCONT);
    }

    executor.setForegroundPid(job->pid);

    int status;
    waitpid(job->pid, &status, WUNTRACED);

    executor.setForegroundPid(0);

    if (WIFSTOPPED(status)) {
        job->state = JobState::stopped;
        std::cout << "\n[Stopped] " << job->command << '\n';
    } else {
        executor.removeJob(jobId);
    }

    return true;
}

bool Builtins::bg(const std::vector<std::string>& args, Executor& executor) {
    if (args.size() < 2) {
        std::cerr << "bg: job ID required\n";
        return false;
    }

    int jobId;
    try {
        jobId = std::stoi(args[1]);
    } catch (...) {
        std::cerr << "bg: invalid job ID: " << args[1] << '\n';
        return false;
    }

    Job* job = executor.findJob(jobId);
    if (!job) {
        std::cerr << "bg: no such job: " << jobId << '\n';
        return false;
    }

    if (job->state == JobState::stopped) {
        kill(job->pid, SIGCONT);
        job->state = JobState::running;
        std::cout << "[" << job->id << "] " << job->command << " &\n";
    }

    return true;
}

bool Builtins::history_cmd(const std::vector<std::string>& args) {
    const auto& hist = history();

    size_t start = 0;
    if (args.size() > 1) {
        try {
            size_t count = static_cast<size_t>(std::stoi(args[1]));
            if (count < hist.size()) {
                start = hist.size() - count;
            }
        } catch (...) {
            std::cerr << "history: invalid count\n";
            return false;
        }
    }

    for (size_t i = start; i < hist.size(); ++i) {
        std::cout << (i + 1) << "  " << hist[i] << '\n';
    }

    return true;
}

} // namespace tinyshell
