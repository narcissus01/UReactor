# UReactor

基于 C++20 的高性能游戏服务器学习项目。

Ubuntu/WSL 构建依赖（堆栈源码定位需要系统库 libdw）：

```bash
sudo apt update
sudo apt install libdw-dev pkg-config
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ctest --test-dir build -R 'test_(config|backtrace|assertion_.*|formatter|log_buffer)$' --output-on-failure
```

断言失败直接写入标准错误，不受 Logger 等级影响。Debug 中止进程；Release
保留断言检查、诊断和 return/break/continue 回退行为。Debug 的源码行号依赖调试信息，
Release 中的内联、优化和缺失调试符号可能减少可见堆栈信息。

第三方源码使用 Git 子模块管理，首次克隆请使用 `git clone --recurse-submodules`，
已有仓库执行 `git submodule update --init --recursive`。

yaml-cpp 是配置系统的必需依赖，普通构建自动编译；其他第三方库默认不参与构建。分支下载、CMake 接入与集成测试命令见
[第三方库与 CMake 学习手册](docs/第三方库与CMake学习手册.md)。
