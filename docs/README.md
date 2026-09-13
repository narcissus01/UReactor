# UReactor 工程结构与构建

本工程按课程的目录组织方式搭建，将 pans 对应为 ureactor。
当前为工程骨架，静态库只有占位翻译单元，尚未实现服务器功能或业务测试。

## 目录

```text
UReactor/
├── CMakeLists.txt
├── docs/README.md
├── ureactor/
│   ├── CMakeLists.txt
│   ├── include/ureactor/       # 对外公开的头文件
│   └── src/                   # 核心实现与私有头文件
├── tests/CMakeLists.txt        # 正确性与健壮性测试
├── tools/CMakeLists.txt        # 辅助工具
├── examples/CMakeLists.txt     # 使用示例
└── cmake/
    ├── UReactorOptions.cmake
    ├── UReactorBuildJobs.cmake
    └── UReactorConfig.cmake.in
```

## 在 WSL 中构建

要求 CMake >= 3.31，支持 C++20 的编译器。当前已验证环境为 Ubuntu 24.04、GCC 13.3、CMake 4.4.3。

```bash
cd /mnt/d/CODE/UReactor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
```

静态库输出为 build/ureactor/libureactor.a。
配置阶段会输出建议并行度：逻辑 CPU 数 × 1.5，并按当前可用内存每个任务约 1.5 GiB 限制，至少为 1。
这个值只是建议；构建时用 --parallel 显式指定，或设置 CMAKE_BUILD_PARALLEL_LEVEL 环境变量。

更换编译器时使用新构建目录，例如：

```bash
cmake -S . -B build-gcc13 -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_COMPILER=g++-13
cmake --build build-gcc13 --parallel 4
```

构建类型可选 Debug、Release、RelWithDebInfo、MinSizeRel，单配置生成器默认 Debug。

## 编译开关

| 开关 | 默认 | 用途 |
|---|---|---|
| UREACTOR_BUILD_TESTS | ON | 添加 tests 子目录 |
| UREACTOR_BUILD_TOOLS | ON | 添加 tools 子目录 |
| UREACTOR_BUILD_EXAMPLES | ON | 添加 examples 子目录 |
| UREACTOR_ENABLE_COVERAGE | OFF | GCC 覆盖率编译及链接选项 |

目前 tests、tools、examples 都是预留目录，不产生可执行程序。添加测试后可执行 ctest --test-dir build --output-on-failure。

只构建核心库：

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DUREACTOR_BUILD_TESTS=OFF -DUREACTOR_BUILD_TOOLS=OFF -DUREACTOR_BUILD_EXAMPLES=OFF
cmake --build build-release --parallel 4
```

覆盖率构建：

```bash
cmake -S . -B build-coverage -DUREACTOR_ENABLE_COVERAGE=ON
cmake --build build-coverage --parallel 4
```

覆盖率开关只添加插桩参数；需要真实测试程序执行代码后才有覆盖率数据。

## 安装和外部使用

将安装结果放在构建目录里，无需 sudo：

```bash
cmake --install build --prefix "$PWD/build/install"
```

外部项目的 CMakeLists.txt：

```cmake
cmake_minimum_required(VERSION 3.31)
project(Consumer LANGUAGES CXX)
find_package(UReactor 0.1 CONFIG REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE UReactor::ureactor)
```

配置外部项目时传入 -DCMAKE_PREFIX_PATH=/mnt/d/CODE/UReactor/build/install。
包会导出 UReactor::ureactor 和 UReactor::options，并自动查找 Threads。

## 与教程的对应及调整

- 使用 C++20，并按教程启用编译器扩展。
- 编译与链接参数通过 ureactor_options 接口库传递，核心库单独启用 -Werror。
- PIC 使用 CMake 的 POSITION_INDEPENDENT_CODE 属性表达；Linux 链接启用 -rdynamic。
- GNU/Clang 使用 -Wall -Wextra -Wpedantic -fno-strict-aliasing，保留栈帧指针；Debug 加入 -g3，GCC 加入 -ggdb。
- 编译器通过配置命令选择，不在项目中用 CACHE FORCE 覆盖用户的工具链设置。
- 按教程使用 GLOB_RECURSE CONFIGURE_DEPENDS 收集代码；新源码放在 ureactor/src，公开头文件放在 ureactor/include/ureactor。
- 安装头文件时保留目录层级，并补齐 Config、Version、Targets 文件，使 find_package 真正可用。
- -fno-strict-aliasing 不会使任意指针转换自动安全，也不能解决未对齐访问、对象生命周期等问题。

## 代码命名约定

- 常量：全大写。
- 类名：大驼峰。
- 非静态成员函数：小驼峰；静态成员函数：大驼峰。
- 非静态成员变量：m_ + 小驼峰；静态成员变量：s_ + 小驼峰。
- 全局函数：大驼峰。
- 函数参数与局部变量：小写下划线。
