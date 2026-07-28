#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

namespace tinyshell {

enum class JobState {
    running,
    stopped,
    done
};

struct Job {
    int id;
    pid_t pid;
    std::string command;
    JobState state;

    Job(int id, pid_t pid, std::string cmd)
        : id(id), pid(pid), command(std::move(cmd)), state(JobState::running) {}
};

} // namespace tinyshell
