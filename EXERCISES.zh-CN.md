# tinyshell 练习手册

这不是"读一遍点头"的练习。每个实验都从一个**因为真实缺陷而失败**的测试开始。
你的任务是让它变绿，同时不破坏 41 个单元测试和端到端会话测试。

## 准备

```bash
# 默认构建：单元测试 + 端到端会话，必须保持全绿
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure

# 练习构建：可选，故意是红的
cmake -S . -B build-exercises -DBUILD_TESTING=ON -DTINYSHELL_BUILD_EXERCISES=ON
cmake --build build-exercises
ctest --test-dir build-exercises -R exercise_01 --output-on-failure
```

一次只做一个实验。`ctest -R exercise_03` 只跑第 3 题；`ctest -E exercise`
跑正常测试集。CTest 会为会话测试设置 `TINYSHELL_BIN`，不用手动设。

## 循环

1. **RED**：跑测试，读失败原因，确认自己能解释它为什么失败。
2. **只改一处**：写最小修改让测试通过。
3. **GREEN**：重跑本题，再跑整个默认测试集。
4. **重构**：在保持全绿的前提下改善命名或去重。
5. 用一句话写下"原代码为什么错"。

不要改练习测试让它通过。如果确信测试写错了，先明确说明，再改。

---

## 练习 01 — 引号里的管道是数据

- **测试**：`exercise_01_quote_aware_pipes`
- **现象**：`echo "a|b"` 被解析成两条命令。
- **阅读**：`src/parser.cpp` 的 `Parser::parse`，它用 `std::getline(..., '|')` 分割，看不见引号。
- **任务**：只在 `|` 不在单引号或双引号内时分割。
- **验收**：pipeline 只有一条命令，第二个参数恰好是 `a|b`。
- **为什么重要**：解析器不认引号，任何带 shell 元字符的命令都会被破坏。这是真 shell 第一件必须做对的事。

## 练习 02 — 反斜杠转义空格

- **测试**：`exercise_02_backslash_escaping`
- **现象**：`echo one\ two` 变成三个 token，转义空格没生效。
- **阅读**：`src/parser.cpp` 的 `Parser::tokenize`。
- **任务**：引号外的反斜杠必须把下一个字符原样纳入 token。自己决定引号内 `\'` 和 `\"` 的语义，并写下来。
- **验收**：`tokenize("echo one\\ two")` 返回 `{"echo", "one two"}`。
- **为什么重要**：引号和转义是两套机制，混用是注入类 bug 的常见来源。

## 练习 03 — 内置命令也要遵守重定向

- **测试**：`exercise_03_builtin_redirects`
- **现象**：`echo captured > file` 写到终端，文件是空的。
- **阅读**：`src/shell.cpp` 的 `Shell::processCommand`。单条内置命令直接交给 `Builtins::execute`，`cmd.outputFile` 根本没被用上。
- **任务**：让内置命令也执行与 `Executor::handleRedirects` 等价的逻辑。想清楚文件描述符属于哪个进程、什么时候恢复 `stdout`。
- **验收**：文件里有 `captured`，普通 `echo hello` 仍输出到终端。
- **为什么重要**："内置命令运行在 shell 进程里"正是这个 bug 容易被漏掉的原因——重定向逻辑在 executor 里，不在 builtin 里。

## 练习 04 — fd 数组太小

- **测试**：`exercise_04_pipeline_fd_array`——**这题以超时作为 RED**。
- **现象**：`printf 'a\nb\n' | wc -l` 永远不结束；fd 数组越界。
- **阅读**：`src/executor.cpp` 的 `Executor::execute`。`pipes` 按 `commands - 1` 分配，却按 `pipes[i*2]` / `pipes[i*2+1]` 索引。
- **任务**：每条管道边分配两个 fd。然后检查父子进程中所有 fd 是否都正确关闭。
- **验收**：管道输出 `2`，没有 sanitizer 或 libc assertion 报错。
- **为什么重要**：这是未定义行为，不是算错答案。应该用调试器或 ASan。再想想为什么 41 个单元测试没抓到——它们只解析管道，从不执行管道。

## 练习 05 — 保留真实退出码

- **测试**：`exercise_05_exact_exit_status`
- **现象**：`sh -c 'exit 7'` 之后 `echo $?` 打印 `1`。
- **阅读**：`Executor::executeCommand` 返回 `bool`；`Shell::processCommand` 把 `success ? 0 : 1` 写进 `$?`。
- **任务**：把真实的 0–255 状态一路传到 shell。只在调用方真的只需要布尔值的地方保留 `bool`。
- **验收**：上述命令后 `$?` 打印 `7`。
- **为什么重要**：把状态码压成成功/失败会丢掉脚本依赖的信息。这跟"把错误码改成布尔值"是同一类错误。

## 练习 06 — 单引号保护变量

- **测试**：`exercise_06_single_quotes_literal`
- **现象**：`echo '$NAME'` 仍然展开 `$NAME`。
- **阅读**：`src/shell.cpp` 的 `Shell::expandVariables`，它扫描整行但不跟踪引号状态。
- **任务**：单引号内不做展开，双引号内仍然展开。自己决定 `\$` 的行为并测试。
- **验收**：`set NAME=student` 后，`echo '$NAME'` 打印 `$NAME`，`echo "$NAME"` 打印 `student`。
- **为什么重要**：展开顺序是 shell 契约的一部分。在解析前展开，解析器就再也看不到用户原始文本。

## 练习 07 — 重定向失败必须中止命令

- **测试**：`exercise_07_redirect_failure_aborts`
- **现象**：`echo should-not-print > /no/such/directory/out` 仍写到 stdout。
- **阅读**：`Executor::handleRedirects` 返回 `bool`，但两个调用方都忽略了它。
- **任务**：重定向失败时在 `exec`/内置命令执行前停下，并让退出码反映失败。
- **验收**：stdout 上不出现 `should-not-print`。
- **为什么重要**：忽略一个 `bool` 返回值看似小事，后果却是输出跑到了用户不期望的地方。

---

## 故意不做的事

这些实验围绕本仓库的系统编程概念，不是要把 shell 做成完整 POSIX 实现。
不要顺手加通配展开、命令替换、readline 或完整作业控制，除非你另外开一个实验。
每次改动要小，并写清楚取舍。
