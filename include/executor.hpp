#pragma once

#include "parser.hpp"
#include "job.hpp"

#include <string>
#include <vector>
#include <map>

namespace tinyshell {

class Executor {
public:
    Executor();
    ~Executor();

    bool execute(const Pipeline& pipeline);
    bool executeCommand(const Command& cmd);
    std::string findExecutable(const std::string& command);

    void addJob(pid_t pid, const std::string& command);
    void removeJob(int id);
    void updateJobStatus();
    const std::map<int, Job>& jobs() const;
    Job* findJob(int id);
    Job* findJobByPid(pid_t pid);
    int nextJobId();

    void setForegroundPid(pid_t pid);
    pid_t foregroundPid() const;

    void onChildStatusChange(int pid, int status);

private:
    bool executeExternal(const Command& cmd, bool background);
    bool handleRedirects(const Command& cmd);

    std::map<int, Job> jobs_;
    int nextJobId_{1};
    pid_t foregroundPid_{0};
};

} // namespace tinyshell
