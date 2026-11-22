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
- Full POSIX job control (we have basic `jobs` / `fg` / `bg` only)
- Supporting complex shell scripting
- Implementing all POSIX shell features

## Features

- Basic external command execution (`fork` / `exec`)
- Built-ins: `cd`, `pwd`, `echo`, `help`, `exit`, `set`, `unset`, `env`,
  `export`, `jobs`, `fg`, `bg`, `history`
- I/O redirection for external commands: `<`, `>`, `>>`
- Pipeline **parsing** for `cmd1 | cmd2 | ...`
- Shell variables: `$VAR`, `${VAR}`, `$?`, `$$`, plus `set NAME=value`
- Command history
- Job tracking and `fg` / `bg` with `SIGCONT`
- Signal handling: SIGINT, SIGTSTP, SIGCHLD, SIGQUIT

## Current Limits

Several of these are deliberate exercise starting points, not oversights:

- Pipes are split without respecting quotes (`echo "a|b"` breaks)
- No backslash escaping in the tokenizer
- Built-ins ignore redirections
- **Pipeline execution is currently broken**: the fd array is indexed out of
  bounds, so even `a | b` can hang. See exercise 04.
- `$?` records only 0/1, not the real exit status
- Variables expand even inside single quotes
- A failed redirection does not abort the command
- No command substitution, glob expansion, or readline-style editing

## Learn by fixing

Work through [EXERCISES.md](EXERCISES.md): seven labs, each with a test that
fails for a real reason and a short explanation of why it matters. The
exercise tests are opt-in (`-DTINYSHELL_BUILD_EXERCISES=ON`), so the normal
build and test suite stay green.

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
$ set NAME=student
$ echo $NAME
$ printf saved > output.txt
$ jobs
$ history
$ exit
```

## Architecture

- `src/shell.cpp` — main loop, variable expansion, command dispatch
- `src/parser.cpp` — tokenization, quotes, redirections, pipes
- `src/executor.cpp` — `fork`/`exec`, descriptors, pipelines, job table
- `src/builtins.cpp` — built-in commands
- `src/signals.cpp` — signal handling setup
- `include/job.hpp` — job table entry
- `tests/tinyshell_tests.cpp` — 41 unit tests
- `tests/e2e_session_tests.cpp` — scripted sessions against the real binary
- `tests/exercises/` — opt-in, intentionally failing labs
