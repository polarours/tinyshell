# tinyshell

A minimal Unix shell implementation in modern C++.

This project is part of the Cppuccino learning roadmap and demonstrates systems programming concepts including process management, file descriptors, and signal handling.

## Learning Goals

- Understand process creation and execution (fork/exec)
- Practice file descriptor manipulation and I/O redirection
- Learn signal handling in a Unix environment
- Implement command parsing and pipeline support
- Apply RAII principles to system resources

## Non-Goals

- Building a production shell
- Implementing job control or background processes
- Supporting complex shell scripting
- Implementing all POSIX shell features

## Features

- Basic command execution (external programs)
- Built-in commands: `cd`, `exit`, `help`, `pwd`, `echo`
- I/O redirection: `<`, `>`, `>>`
- Pipeline support: `cmd1 | cmd2 | cmd3`
- Command line editing (basic)
- Command history (basic)
- Signal handling (Ctrl+C, Ctrl+Z)

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/tinyshell
```

## Example Session

```text
$ ls -la
$ echo "Hello World"
$ pwd
$ cd /tmp
$ cat file.txt | grep "pattern" | wc -l
$ ls > output.txt
$ sort < input.txt > sorted.txt
$ exit
```

## Architecture

- `src/shell.cpp` - Main shell loop and command processing
- `src/parser.cpp` - Command line parsing and tokenization
- `src/executor.cpp` - Process creation and execution
- `src/builtins.cpp` - Built-in command implementations
- `src/signals.cpp` - Signal handling setup

## Current Limits

- No job control or background processes
- Limited line editing capabilities
- No shell variable expansion
- No command substitution
- Basic error handling

## Suggested Next Steps

- Add job control (fg, bg, jobs)
- Implement shell variables and environment handling
- Add command substitution support
- Improve line editing with readline or similar
- Add exit status handling
