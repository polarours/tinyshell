# TinyShell

一个用现代 C++ 实现的最小 Unix shell。

本项目是 Cppuccino 学习路线图的一部分，演示了系统编程概念，
包括进程管理、文件描述符和信号处理。

## 学习目标

- 理解进程创建和执行（fork/exec）
- 练习文件描述符操作和 I/O 重定向
- 学习 Unix 环境中的信号处理
- 实现命令解析和管道支持
- 将 RAII 原则应用于系统资源

## 非目标

- 构建生产级 shell
- 完整的 POSIX 作业控制（目前只有基础的 `jobs` / `fg` / `bg`）
- 支持复杂的 shell 脚本
- 实现所有 POSIX shell 特性

## 功能

- **内置命令**: `cd`, `pwd`, `echo`, `help`, `exit`, `set`, `unset`, `env`, `export`, `jobs`, `fg`, `bg`, `history`
- **I/O 重定向**: `<`, `>`, `>>`
- **管道**: `|` 解析（执行目前有已知缺陷，见当前限制）
- **引号处理**: 单引号和双引号
- **信号处理**: SIGINT, SIGTSTP, SIGCHLD, SIGQUIT
- **Shell 变量**: `set`, `unset`, `env`, `export`
- **变量展开**: `$VAR`, `${VAR}`, `$?`, `$$`
- **作业控制**: `jobs`, `fg`, `bg`（基础版）

## 当前限制

其中不少是故意留作练习起点的，不是疏忽：

- 管道分割不认引号（`echo "a|b"` 会坏）
- tokenizer 不支持反斜杠转义
- 内置命令忽略重定向
- **管道执行目前是坏的**：fd 数组越界，连 `a | b` 都可能挂住，见练习 04
- `$?` 只记录 0/1，不是真实退出码
- 单引号里仍然展开变量
- 重定向失败不会中止命令
- 没有命令替换、通配展开和 readline 式编辑

## 边修边学

[EXERCISES.zh-CN.md](EXERCISES.zh-CN.md) 有 7 个实验，每个都从一个因为真实
缺陷而失败的测试开始，并解释它为什么重要。练习测试是可选的
（`-DTINYSHELL_BUILD_EXERCISES=ON`），默认构建和测试集保持全绿。

## 构建

```bash
cmake -S . -B build
cmake --build build
```

## 运行

```bash
./build/tinyshell
```

## 测试

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## 架构

```
tinyshell/
├── src/
│   ├── shell.cpp       # 主循环、变量展开、命令分发
│   ├── parser.cpp      # tokenize、引号、重定向、管道
│   ├── executor.cpp    # fork/exec、fd、管道、作业表
│   ├── builtins.cpp    # 内置命令
│   └── signals.cpp     # 信号处理
├── include/
│   ├── shell.hpp
│   ├── parser.hpp
│   ├── executor.hpp
│   ├── builtins.hpp
│   ├── job.hpp
│   └── signals.hpp
└── tests/
    ├── tinyshell_tests.cpp       # 41 个单元测试
    ├── e2e_session_tests.cpp     # 真实二进制的脚本化会话
    └── exercises/                # 可选、故意失败的 7 个实验
```
