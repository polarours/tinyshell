# tinyshell exercises

These are not "read and nod" exercises. Each lab starts from a test that
**fails for a real reason** in the current code. Your job is to make it green
without breaking the 41 unit tests or the end-to-end session tests.

## Setup

```bash
# Default build: unit tests + end-to-end sessions. Must stay green.
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure

# Exercise build: opt-in, intentionally RED.
cmake -S . -B build-exercises -DBUILD_TESTING=ON -DTINYSHELL_BUILD_EXERCISES=ON
cmake --build build-exercises
ctest --test-dir build-exercises -R exercise_01 --output-on-failure
```

Run one lab at a time. `ctest -R exercise_03` runs only lab 3; `ctest -E exercise`
runs the normal suite. CTest sets `TINYSHELL_BIN` for the session harness, so
you do not need to set it by hand.

## The loop

1. **RED**: run the lab test, read the failure, and make sure you can explain it.
2. **Change one thing**: write the smallest fix that could make that test pass.
3. **GREEN**: rerun the lab, then the whole default suite.
4. **Refactor**: clean up naming or duplication while everything stays green.
5. Explain in one sentence why the original code failed. Write it down.

Do not edit the exercise tests to make them pass. If a test looks wrong,
say so explicitly before changing it.

---

## Lab 01 — A quoted pipe is data

- **Test**: `exercise_01_quote_aware_pipes`
- **Symptom**: `echo "a|b"` is parsed as two commands.
- **Read**: `Parser::parse` in `src/parser.cpp`. It calls `std::getline(stream, cmdStr, '|')`, which cannot see quotes.
- **Task**: split on `|` only when the character is outside both `'` and `"`.
- **Acceptance**: the pipeline has one command whose second argument is exactly `a|b`.
- **Why it matters**: a parser that does not respect quotes will corrupt any command containing shell metacharacters. This is the first thing a real shell must get right.

## Lab 02 — Backslash escapes a space

- **Test**: `exercise_02_backslash_escaping`
- **Symptom**: `echo one\ two` becomes three tokens; the escaped space splits the argument.
- **Read**: `Parser::tokenize` in `src/parser.cpp`.
- **Task**: when outside quotes, a backslash must consume the next character literally. Decide what `\'` and `\"` should do inside quotes and document your choice.
- **Acceptance**: `tokenize("echo one\\ two")` returns `{"echo", "one two"}`.
- **Why it matters**: quoting and escaping are two different mechanisms. Mixing them up is a classic source of injection bugs.

## Lab 03 — Builtins must honour redirection

- **Test**: `exercise_03_builtin_redirects`
- **Symptom**: `echo captured > file` writes to the terminal; the file stays empty.
- **Read**: `Shell::processCommand` in `src/shell.cpp`. Single builtins go straight to `Builtins::execute`, so `cmd.outputFile` is never applied.
- **Task**: apply `Executor::handleRedirects` semantics to builtins too. Think about which process owns the file descriptor and when to restore `stdout`.
- **Acceptance**: the redirected file contains `captured`, and a plain `echo hello` still reaches the terminal.
- **Why it matters**: "builtins run in the shell process" is exactly why this bug is easy to miss — the redirect code lives in the executor, not in the builtin.

## Lab 04 — The fd array is too small

- **Test**: `exercise_04_pipeline_fd_array` — **this lab fails by timing out**.
- **Symptom**: `printf 'a\nb\n' | wc -l` never finishes; the fd vector is indexed out of bounds.
- **Read**: `Executor::execute` in `src/executor.cpp`. `pipes` is sized as `commands - 1`, but the code indexes `pipes[i*2]` and `pipes[i*2+1]`.
- **Task**: allocate two descriptors per pipe edge. Then check that every descriptor is closed in every child and in the parent.
- **Acceptance**: the pipeline prints `2`, and no sanitizer or libc assertion fires.
- **Why it matters**: this is undefined behaviour, not a wrong answer. A debugger or ASan is the right tool. Ask yourself why the 41 unit tests never caught it — they only parse pipelines, they never execute them.

## Lab 05 — Preserve the exact exit status

- **Test**: `exercise_05_exact_exit_status`
- **Symptom**: `sh -c 'exit 7'` followed by `echo $?` prints `1`.
- **Read**: `Executor::executeCommand` returns `bool`; `Shell::processCommand` stores `success ? 0 : 1` in `$?`.
- **Task**: carry the real 0–255 status through the executor and shell. Keep `bool` only where a boolean is actually what callers need.
- **Acceptance**: `$?` prints `7` after that command.
- **Why it matters**: collapsing a status to success/failure destroys information that scripts depend on. This is the same class of bug as turning an error code into a boolean.

## Lab 06 — Single quotes protect variables

- **Test**: `exercise_06_single_quotes_literal`
- **Symptom**: `echo '$NAME'` expands `$NAME` anyway.
- **Read**: `Shell::expandVariables` in `src/shell.cpp`. It scans the whole line without tracking quote state.
- **Task**: skip expansion inside single quotes. Double quotes should still expand. Decide what happens to `\$` and test your decision.
- **Acceptance**: after `set NAME=student`, `echo '$NAME'` prints `$NAME`, while `echo "$NAME"` prints `student`.
- **Why it matters**: expansion order is part of a shell's contract. Doing it before parsing means the parser never sees the user's original text.

## Lab 07 — A failed redirect must abort

- **Test**: `exercise_07_redirect_failure_aborts`
- **Symptom**: `echo should-not-print > /no/such/directory/out` still writes to stdout.
- **Read**: `Executor::handleRedirects` returns `bool`, but both callers ignore it.
- **Task**: stop before `exec`/builtin execution when a redirect fails, and make the failure visible in the exit status.
- **Acceptance**: `should-not-print` never appears on stdout.
- **Why it matters**: ignoring a returned `bool` is a small mistake with a security-relevant consequence: output goes somewhere the user did not intend.

---

## What is deliberately out of scope

These labs are about the systems concepts in this repository, not about
building a POSIX shell. Do not add glob expansion, command substitution,
readline, or full job-control semantics unless you open a separate exercise
for them. Keep the change small and explain the tradeoff.
