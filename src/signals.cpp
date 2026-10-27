#include "signals.hpp"
#include <cerrno>
#include <unistd.h>
#include <sys/wait.h>

namespace tinyshell {

Signals::ChildStatusCallback Signals::childStatusCallback_ = nullptr;
volatile sig_atomic_t Signals::foregroundPid_ = 0;
volatile sig_atomic_t Signals::gotSigchld_ = 0;

void Signals::handleSigint(int signum) {
    (void)signum;

    // A handler runs in the middle of whatever the program was doing, and
    // the calls below may set errno. The value the interrupted code had is
    // put back, so that a handler cannot change what the program sees.
    const int savedErrno = errno;

    if (foregroundPid_ > 0) {
        kill(foregroundPid_, SIGINT);
    } else {
        const char msg[] = "\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    }

    errno = savedErrno;
}

void Signals::handleSigtstp(int signum) {
    (void)signum;

    const int savedErrno = errno;

    if (foregroundPid_ > 0) {
        kill(foregroundPid_, SIGTSTP);
    }

    errno = savedErrno;
}

void Signals::handleSigchld(int signum) {
    (void)signum;
    gotSigchld_ = 1;

    const int savedErrno = errno;

    int status;
    int pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (childStatusCallback_) {
            childStatusCallback_(pid, status);
        }
    }

    errno = savedErrno;
}

void Signals::handleSigquit(int signum) {
    (void)signum;

    const int savedErrno = errno;

    if (foregroundPid_ > 0) {
        kill(foregroundPid_, SIGQUIT);
    } else {
        _exit(0);
    }

    errno = savedErrno;
}

void Signals::setup() {
    struct sigaction sa;

    // SIGINT (Ctrl+C)
    sa.sa_handler = handleSigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);

    // SIGTSTP (Ctrl+Z)
    sa.sa_handler = handleSigtstp;
    sigaction(SIGTSTP, &sa, nullptr);

    // SIGCHLD (child process status change)
    sa.sa_handler = handleSigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, nullptr);

    // SIGQUIT (Ctrl+\)
    sa.sa_handler = handleSigquit;
    sigaction(SIGQUIT, &sa, nullptr);

    // Ignore SIGTTIN and SIGTTOU for background job control
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

void Signals::reset() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

void Signals::setChildStatusCallback(ChildStatusCallback callback) {
    childStatusCallback_ = std::move(callback);
}

void Signals::setForegroundPid(int pid) {
    foregroundPid_ = pid;
}

int Signals::foregroundPid() {
    return foregroundPid_;
}

} // namespace tinyshell
