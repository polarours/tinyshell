#include "parser.hpp"
#include "builtins.hpp"
#include "executor.hpp"
#include "job.hpp"
#include "signals.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_parser_simple_command() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("ls -la");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].args.size() == 2, "expected 2 args");
    expect(pipeline.commands[0].args[0] == "ls", "expected 'ls'");
    expect(pipeline.commands[0].args[1] == "-la", "expected '-la'");
}

void test_parser_pipeline() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("cat file.txt | grep pattern | wc -l");

    expect(pipeline.commands.size() == 3, "expected 3 commands in pipeline");
    expect(pipeline.commands[0].args[0] == "cat", "expected 'cat'");
    expect(pipeline.commands[0].args[1] == "file.txt", "expected 'file.txt'");
    expect(pipeline.commands[1].args[0] == "grep", "expected 'grep'");
    expect(pipeline.commands[1].args[1] == "pattern", "expected 'pattern'");
    expect(pipeline.commands[2].args[0] == "wc", "expected 'wc'");
    expect(pipeline.commands[2].args[1] == "-l", "expected '-l'");
}

void test_parser_input_redirect() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("sort < input.txt");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].args.size() == 1, "expected 1 arg");
    expect(pipeline.commands[0].args[0] == "sort", "expected 'sort'");
    expect(pipeline.commands[0].inputFile == "input.txt", "expected input file");
}

void test_parser_output_redirect() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("ls > output.txt");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].outputFile == "output.txt", "expected output file");
    expect(!pipeline.commands[0].appendOutput, "expected truncate mode");
}

void test_parser_append_redirect() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("echo 'line' >> output.txt");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].outputFile == "output.txt", "expected output file");
    expect(pipeline.commands[0].appendOutput, "expected append mode");
}

void test_parser_combined_redirects() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("sort < input.txt > sorted.txt");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].inputFile == "input.txt", "expected input file");
    expect(pipeline.commands[0].outputFile == "sorted.txt", "expected output file");
}

void test_parser_quotes() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("echo \"hello world\"");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].args.size() == 2, "expected 2 args");
    expect(pipeline.commands[0].args[1] == "hello world", "expected quoted string");
}

void test_parser_single_quotes() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("echo 'no expansion'");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].args[1] == "no expansion", "expected single quoted string");
}

void test_parser_empty_input() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("");

    expect(pipeline.commands.empty(), "expected empty pipeline");
}

void test_parser_whitespace_only() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("   \t  ");

    expect(pipeline.commands.empty(), "expected empty pipeline for whitespace");
}

void test_parser_pipe_errors() {
    tinyshell::Parser parser;

    bool threw1 = false;
    try { parser.parse("ls > "); } catch (...) { threw1 = true; }
    expect(threw1, "expected error for missing output file");

    bool threw2 = false;
    try { parser.parse("sort < "); } catch (...) { threw2 = true; }
    expect(threw2, "expected error for missing input file");
}

void test_builtins_is_builtin() {
    tinyshell::Builtins builtins;

    expect(builtins.isBuiltin("cd"), "expected cd to be builtin");
    expect(builtins.isBuiltin("pwd"), "expected pwd to be builtin");
    expect(builtins.isBuiltin("echo"), "expected echo to be builtin");
    expect(builtins.isBuiltin("help"), "expected help to be builtin");
    expect(builtins.isBuiltin("exit"), "expected exit to be builtin");
    expect(!builtins.isBuiltin("ls"), "expected ls to not be builtin");
    expect(!builtins.isBuiltin("cat"), "expected cat to not be builtin");
}

void test_builtins_current_directory() {
    tinyshell::Builtins builtins;
    std::string dir = builtins.currentDirectory();

    expect(!dir.empty(), "expected non-empty current directory");
    expect(dir[0] == '/', "expected absolute path");
}

void test_builtins_cd() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;
    std::string originalDir = builtins.currentDirectory();

    // cd to /tmp
    bool result = builtins.execute({"cd", "/tmp"}, executor);
    expect(result, "expected cd to succeed");
    expect(builtins.currentDirectory() == "/tmp", "expected directory to be /tmp");

    // cd back
    builtins.execute({"cd", originalDir.c_str()}, executor);
}

void test_builtins_cd_home() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;
    const char* home = getenv("HOME");
    if (!home) {
        return;
    }

    builtins.execute({"cd"}, executor);
    expect(builtins.currentDirectory() == home, "expected HOME directory");
}

void test_builtins_pwd() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    bool result = builtins.execute({"pwd"}, executor);
    expect(result, "expected pwd to succeed");
}

void test_builtins_echo() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    bool result = builtins.execute({"echo", "hello", "world"}, executor);
    expect(result, "expected echo to succeed");
}

void test_builtins_help() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    bool result = builtins.execute({"help"}, executor);
    expect(result, "expected help to succeed");
}

void test_executor_find_executable() {
    tinyshell::Executor executor;

    // ls should be found
    std::string lsPath = executor.findExecutable("ls");
    expect(!lsPath.empty(), "expected 'ls' to be found");
    expect(lsPath.find("ls") != std::string::npos, "expected path to contain 'ls'");

    // nonexistent command should not be found
    std::string noPath = executor.findExecutable("nonexistent_command_12345");
    expect(noPath.empty(), "expected nonexistent command to not be found");
}

void test_executor_execute_single() {
    tinyshell::Executor executor;
    tinyshell::Command cmd;
    cmd.args = {"echo", "test"};

    bool result = executor.executeCommand(cmd);
    expect(result, "expected echo to succeed");
}

void test_executor_execute_empty() {
    tinyshell::Executor executor;
    tinyshell::Pipeline pipeline;

    bool result = executor.execute(pipeline);
    expect(result, "expected empty pipeline to succeed");
}

void test_builtins_set_variable() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    expect(builtins.execute({"set", "myvar=hello"}, executor), "expected set to succeed");
    expect(builtins.getVariable("myvar") == "hello", "expected variable value");

    expect(builtins.execute({"set", "numvar=42"}, executor), "expected set to succeed");
    expect(builtins.getVariable("numvar") == "42", "expected numeric variable");
}

void test_builtins_unset_variable() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    builtins.setVariable("temp", "value");
    expect(builtins.getVariable("temp") == "value", "expected variable to exist");

    expect(builtins.execute({"unset", "temp"}, executor), "expected unset to succeed");
    expect(builtins.getVariable("temp").empty(), "expected variable to be gone");
}

void test_builtins_env() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    builtins.setVariable("test_env", "test_value");
    bool result = builtins.execute({"env"}, executor);
    expect(result, "expected env to succeed");
}

void test_builtins_exit_status() {
    tinyshell::Builtins builtins;

    expect(builtins.lastExitStatus() == 0, "expected initial exit status 0");

    builtins.setLastExitStatus(42);
    expect(builtins.lastExitStatus() == 42, "expected exit status 42");
    expect(builtins.getVariable("?") == "42", "expected $? to be 42");

    builtins.setLastExitStatus(0);
    expect(builtins.getVariable("?") == "0", "expected $? to be 0");
}

void test_builtins_set_list() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    builtins.setVariable("a", "1");
    builtins.setVariable("b", "2");

    bool result = builtins.execute({"set"}, executor);
    expect(result, "expected set to succeed");
}

void test_builtins_export() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    builtins.setVariable("MY_EXPORT_VAR", "exported_value");
    bool result = builtins.execute({"export", "MY_EXPORT_VAR"}, executor);
    expect(result, "expected export to succeed");
}

void test_builtins_new_is_builtin() {
    tinyshell::Builtins builtins;

    expect(builtins.isBuiltin("set"), "expected set to be builtin");
    expect(builtins.isBuiltin("unset"), "expected unset to be builtin");
    expect(builtins.isBuiltin("env"), "expected env to be builtin");
    expect(builtins.isBuiltin("export"), "expected export to be builtin");
    expect(builtins.isBuiltin("jobs"), "expected jobs to be builtin");
    expect(builtins.isBuiltin("fg"), "expected fg to be builtin");
    expect(builtins.isBuiltin("bg"), "expected bg to be builtin");
    expect(builtins.isBuiltin("history"), "expected history to be builtin");
}

void test_parser_background() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("sleep 10 &");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].args.size() == 2, "expected 2 args");
    expect(pipeline.commands[0].args[0] == "sleep", "expected 'sleep'");
    expect(pipeline.commands[0].args[1] == "10", "expected '10'");
    expect(pipeline.commands[0].background, "expected background flag");
}

void test_parser_background_with_redirect() {
    tinyshell::Parser parser;
    auto pipeline = parser.parse("echo hello > file.txt &");

    expect(pipeline.commands.size() == 1, "expected 1 command");
    expect(pipeline.commands[0].background, "expected background flag");
    expect(pipeline.commands[0].outputFile == "file.txt", "expected output file");
}

void test_builtins_history() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    builtins.addHistory("ls -la");
    builtins.addHistory("pwd");
    builtins.addHistory("echo hello");

    const auto& hist = builtins.history();
    expect(hist.size() == 3, "expected 3 history entries");
    expect(hist[0] == "ls -la", "expected first entry");
    expect(hist[2] == "echo hello", "expected last entry");

    bool result = builtins.execute({"history"}, executor);
    expect(result, "expected history to succeed");
}

void test_builtins_history_no_duplicates() {
    tinyshell::Builtins builtins;

    builtins.addHistory("ls");
    builtins.addHistory("ls");

    const auto& hist = builtins.history();
    expect(hist.size() == 1, "expected 1 history entry (no duplicates)");
}

void test_builtins_jobs_empty() {
    tinyshell::Builtins builtins;
    tinyshell::Executor executor;

    bool result = builtins.execute({"jobs"}, executor);
    expect(result, "expected jobs to succeed");
}

void test_executor_job_tracking() {
    tinyshell::Executor executor;

    executor.addJob(12345, "sleep 10");
    expect(executor.jobs().size() == 1, "expected 1 job");

    tinyshell::Job* job = executor.findJob(1);
    expect(job != nullptr, "expected to find job");
    expect(job->pid == 12345, "expected correct pid");
    expect(job->command == "sleep 10", "expected correct command");

    executor.removeJob(1);
    expect(executor.jobs().empty(), "expected no jobs after remove");
}

void test_executor_find_job_by_pid() {
    tinyshell::Executor executor;

    executor.addJob(11111, "cmd1");
    executor.addJob(22222, "cmd2");

    tinyshell::Job* job = executor.findJobByPid(22222);
    expect(job != nullptr, "expected to find job by pid");
    expect(job->id == 2, "expected job id 2");

    tinyshell::Job* notFound = executor.findJobByPid(99999);
    expect(notFound == nullptr, "expected nullptr for nonexistent pid");
}

void test_signals_setup() {
    tinyshell::Signals::setup();

    struct sigaction sa;
    sigaction(SIGINT, nullptr, &sa);
    expect(sa.sa_handler != SIG_DFL, "expected SIGINT handler to be set");
    expect(sa.sa_handler != SIG_IGN, "expected SIGINT handler not to be ignored");

    tinyshell::Signals::reset();
}

void test_signals_foreground_pid() {
    tinyshell::Signals::setup();

    tinyshell::Signals::setForegroundPid(12345);
    expect(tinyshell::Signals::foregroundPid() == 12345, "expected foreground pid 12345");

    tinyshell::Signals::setForegroundPid(0);
    expect(tinyshell::Signals::foregroundPid() == 0, "expected foreground pid 0");

    tinyshell::Signals::reset();
}

void test_signals_child_callback() {
    tinyshell::Signals::setup();

    bool callbackCalled = false;
    int callbackPid = 0;

    tinyshell::Signals::setChildStatusCallback([&](int pid, int /*status*/) {
        callbackCalled = true;
        callbackPid = pid;
    });

    pid_t child = fork();
    if (child == 0) {
        _exit(0);
    }

    int status;
    waitpid(child, &status, 0);

    tinyshell::Signals::setChildStatusCallback(nullptr);
    tinyshell::Signals::reset();
}

void test_executor_on_child_status() {
    tinyshell::Executor executor;

    executor.addJob(99999, "test_cmd");
    expect(executor.jobs().size() == 1, "expected 1 job");

    executor.onChildStatusChange(99999, 0);
    expect(executor.jobs().empty(), "expected job removed after exit");
}

void test_executor_on_child_status_signal() {
    tinyshell::Executor executor;

    executor.addJob(88888, "signal_cmd");
    expect(executor.jobs().size() == 1, "expected 1 job");

    executor.onChildStatusChange(88888, WTERMSIG(SIGTERM));
    expect(executor.jobs().empty(), "expected job removed after signal");
}

void test_executor_foreground_pid_sync() {
    tinyshell::Executor executor;

    executor.setForegroundPid(12345);
    expect(executor.foregroundPid() == 12345, "expected executor foreground pid");
    expect(tinyshell::Signals::foregroundPid() == 12345, "expected signals foreground pid");

    executor.setForegroundPid(0);
    expect(executor.foregroundPid() == 0, "expected executor foreground pid 0");
    expect(tinyshell::Signals::foregroundPid() == 0, "expected signals foreground pid 0");
}

} // namespace

int main() {
    try {
        test_parser_simple_command();
        test_parser_pipeline();
        test_parser_input_redirect();
        test_parser_output_redirect();
        test_parser_append_redirect();
        test_parser_combined_redirects();
        test_parser_quotes();
        test_parser_single_quotes();
        test_parser_empty_input();
        test_parser_whitespace_only();
        test_parser_pipe_errors();
        test_builtins_is_builtin();
        test_builtins_current_directory();
        test_builtins_cd();
        test_builtins_cd_home();
        test_builtins_pwd();
        test_builtins_echo();
        test_builtins_help();
        test_executor_find_executable();
        test_executor_execute_single();
        test_executor_execute_empty();
        test_builtins_set_variable();
        test_builtins_unset_variable();
        test_builtins_env();
        test_builtins_exit_status();
        test_builtins_set_list();
        test_builtins_export();
        test_builtins_new_is_builtin();
        test_parser_background();
        test_parser_background_with_redirect();
        test_builtins_history();
        test_builtins_history_no_duplicates();
        test_builtins_jobs_empty();
        test_executor_job_tracking();
        test_executor_find_job_by_pid();
        test_signals_setup();
        test_signals_foreground_pid();
        test_signals_child_callback();
        test_executor_on_child_status();
        test_executor_on_child_status_signal();
        test_executor_foreground_pid_sync();
    } catch (const std::exception& exception) {
        std::cerr << "tinyshell_tests failed: " << exception.what() << std::endl;
        return 1;
    }

    std::cout << "tinyshell_tests passed" << std::endl;
    return 0;
}
