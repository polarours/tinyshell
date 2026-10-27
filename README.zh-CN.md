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
- 实现作业控制或后台进程
- 支持复杂的 shell 脚本
- 实现所有 POSIX shell 特性

## 功能

- **内置命令**: `cd`, `pwd`, `echo`, `help`, `exit`
- **I/O 重定向**: `<`, `>`, `>>`
- **管道**: `|` 管道支持
- **引号处理**: 单引号和双引号
- **信号处理**: SIGINT, SIGQUIT
- **Shell 变量**: `set`, `unset`, `env`, `export`
- **变量展开**: `$VAR`, `${VAR}`, `$?`, `$$`
- **作业控制**: `jobs`, `fg`, `bg`

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
│   ├── parser.cpp      # 命令解析器
│   ├── executor.cpp     # 命令执行器
│   ├── builtins.cpp     # 内置命令
│   └── signals.cpp      # 信号处理
├── include/
│   ├── parser.hpp
│   ├── executor.hpp
│   ├── builtins.hpp
│   └── signals.hpp
└── tests/
    ├── parser_tests.cpp
    ├── executor_tests.cpp
    ├── builtins_tests.cpp
    └── signals_tests.cpp
```
