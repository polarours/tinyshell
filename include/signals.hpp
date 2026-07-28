#pragma once

#include <csignal>
#include <functional>

namespace tinyshell {

class Signals {
public:
    using ChildStatusCallback = std::function<void(int pid, int status)>;

    static void setup();
    static void reset();

    static void setChildStatusCallback(ChildStatusCallback callback);
    static void setForegroundPid(int pid);
    static int foregroundPid();

private:
    static void handleSigint(int signum);
    static void handleSigtstp(int signum);
    static void handleSigchld(int signum);
    static void handleSigquit(int signum);

    static ChildStatusCallback childStatusCallback_;
    static volatile sig_atomic_t foregroundPid_;
    static volatile sig_atomic_t gotSigchld_;
};

} // namespace tinyshell
