#include "executor.hpp"
#include "signals.hpp"

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <algorithm>
#include <sstream>

namespace tinyshell {

Executor::Executor() {
    Signals::setChildStatusCallback([this](int pid, int status) {
        onChildStatusChange(pid, status);
    });
}

Executor::~Executor() {}

bool Executor::execute(const Pipeline& pipeline) {
    if (pipeline.commands.empty()) {
        return true;
    }

    if (pipeline.commands.size() == 1) {
        return executeCommand(pipeline.commands[0]);
    }

    int numCommands = pipeline.commands.size();
    std::vector<int> pipes(numCommands - 1);

    for (int i = 0; i < numCommands - 1; ++i) {
        if (pipe(&pipes[i * 2]) == -1) {
            perror("pipe");
            return false;
        }
    }

    std::vector<pid_t> pids(numCommands);
    bool isBackground = pipeline.commands.back().background;

    for (int i = 0; i < numCommands; ++i) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork");
            return false;
        }

        if (pids[i] == 0) {
            setpgid(0, 0);

            if (i > 0) {
                dup2(pipes[(i - 1) * 2], STDIN_FILENO);
            }

            if (i < numCommands - 1) {
                dup2(pipes[i * 2 + 1], STDOUT_FILENO);
            }

            for (int fd : pipes) {
                close(fd);
            }

            handleRedirects(pipeline.commands[i]);
            executeExternal(pipeline.commands[i], false);
            _exit(1);
        }
    }

    for (int fd : pipes) {
        close(fd);
    }

    if (isBackground) {
        std::string cmdStr;
        for (const auto& cmd : pipeline.commands) {
            for (const auto& arg : cmd.args) {
                if (!cmdStr.empty()) cmdStr += " ";
                cmdStr += arg;
            }
        }
        addJob(pids.back(), cmdStr + " &");
        return true;
    }

    Signals::setForegroundPid(pids.back());

    for (pid_t pid : pids) {
        int status;
        waitpid(pid, &status, WUNTRACED);
    }

    Signals::setForegroundPid(0);

    return true;
}

bool Executor::executeCommand(const Command& cmd) {
    if (cmd.args.empty()) {
        return true;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        setpgid(0, 0);

        handleRedirects(cmd);
        executeExternal(cmd, false);
        _exit(1);
    }

    if (cmd.background) {
        std::string cmdStr;
        for (const auto& arg : cmd.args) {
            if (!cmdStr.empty()) cmdStr += " ";
            cmdStr += arg;
        }
        addJob(pid, cmdStr);
        return true;
    }

    Signals::setForegroundPid(pid);

    int status;
    waitpid(pid, &status, WUNTRACED);

    Signals::setForegroundPid(0);

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool Executor::executeExternal(const Command& cmd, bool /*background*/) {
    if (cmd.args.empty()) {
        return false;
    }

    std::string executable = findExecutable(cmd.args[0]);
    if (executable.empty()) {
        std::cerr << "Command not found: " << cmd.args[0] << '\n';
        return false;
    }

    std::vector<char*> argv;
    for (const auto& arg : cmd.args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    execvp(executable.c_str(), argv.data());
    perror("execvp");
    return false;
}

bool Executor::handleRedirects(const Command& cmd) {
    if (!cmd.inputFile.empty()) {
        int fd = open(cmd.inputFile.c_str(), O_RDONLY);
        if (fd == -1) {
            perror("open input file");
            return false;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (!cmd.outputFile.empty()) {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd.appendOutput ? O_APPEND : O_TRUNC;
        int fd = open(cmd.outputFile.c_str(), flags, 0644);
        if (fd == -1) {
            perror("open output file");
            return false;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    return true;
}

std::string Executor::findExecutable(const std::string& command) {
    if (command.find('/') != std::string::npos) {
        if (access(command.c_str(), X_OK) == 0) {
            return command;
        }
        return "";
    }

    const char* pathEnv = getenv("PATH");
    if (!pathEnv) {
        return "";
    }

    std::istringstream pathStream(pathEnv);
    std::string dir;

    while (std::getline(pathStream, dir, ':')) {
        std::string fullPath = dir + "/" + command;
        if (access(fullPath.c_str(), X_OK) == 0) {
            return fullPath;
        }
    }

    return "";
}

void Executor::addJob(pid_t pid, const std::string& command) {
    int id = nextJobId_++;
    jobs_.emplace(id, Job(id, pid, command));
    std::cout << "[" << id << "] " << pid << '\n';
}

void Executor::removeJob(int id) {
    jobs_.erase(id);
}

void Executor::updateJobStatus() {
    for (auto it = jobs_.begin(); it != jobs_.end(); ) {
        int status;
        pid_t result = waitpid(it->second.pid, &status, WNOHANG | WUNTRACED);

        if (result > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                it->second.state = JobState::done;
                std::cout << "[" << it->second.id << "] Done\t" << it->second.command << '\n';
                it = jobs_.erase(it);
                continue;
            } else if (WIFSTOPPED(status)) {
                it->second.state = JobState::stopped;
            }
        }

        ++it;
    }
}

const std::map<int, Job>& Executor::jobs() const {
    return jobs_;
}

Job* Executor::findJob(int id) {
    auto it = jobs_.find(id);
    return (it != jobs_.end()) ? &it->second : nullptr;
}

Job* Executor::findJobByPid(pid_t pid) {
    for (auto& [id, job] : jobs_) {
        if (job.pid == pid) {
            return &job;
        }
    }
    return nullptr;
}

int Executor::nextJobId() {
    return nextJobId_;
}

void Executor::setForegroundPid(pid_t pid) {
    foregroundPid_ = pid;
    Signals::setForegroundPid(pid);
}

pid_t Executor::foregroundPid() const {
    return foregroundPid_;
}

void Executor::onChildStatusChange(int pid, int status) {
    Job* job = findJobByPid(pid);
    if (job) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            int exitStatus = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            std::cout << "\n[" << job->id << "] Done\t" << job->command;
            if (exitStatus != 0) {
                std::cout << " (exit status: " << exitStatus << ")";
            }
            std::cout << '\n';
            removeJob(job->id);
        } else if (WIFSTOPPED(status)) {
            job->state = JobState::stopped;
            std::cout << "\n[" << job->id << "] Stopped\t" << job->command << '\n';
        }
    }
}

} // namespace tinyshell
