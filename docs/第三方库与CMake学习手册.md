# 第三方库与 CMake 学习手册

源码位置：`D:\CODE\UReactor\third_party`，WSL 中为 `/mnt/d/CODE/UReactor/third_party`。
构建产物放在项目的 `build-deps` 中，均位于 D 盘，不需要 `sudo make install`。

## 1. Git 管源码，CMake 管构建

Git submodule（子模块）将外部 Git 仓库放进主项目。主仓库记录的是：

- `.gitmodules`：子模块路径、远端 URL、用于更新的分支。
- gitlink：每个子模块的**确切提交 SHA**，不是把整个第三方源码复制进主仓库。
- `.git/modules/third_party/...`：本机保存的子模块 Git 数据，不提交此目录。

`-b master` 指定跟踪分支，不表示每次构建自动更新，也不表示跟踪一个 tag。
主项目提交 SHA 后，协作者可以恢复相同源码版本。升级分支可能产生不兼容变更，必须重新测试。

## 2. 按分支下载

先查看远端默认分支，不要假设所有仓库都叫 master：

```bash
cd /mnt/d/CODE/UReactor
git ls-remote --symref https://github.com/protocolbuffers/protobuf.git HEAD
```

输出 `ref: refs/heads/main HEAD` 就说明默认分支是 main。
添加命令示例（已添加的子模块不要再执行 add）：

```bash
git submodule add --depth 1 -b master https://github.com/jbeder/yaml-cpp.git third_party/yaml-cpp
git submodule add --depth 1 -b master https://github.com/abseil/abseil-cpp.git third_party/abseil-cpp
git submodule add --depth 1 -b main https://github.com/protocolbuffers/protobuf.git third_party/protobuf
git submodule add --depth 1 -b master https://github.com/Tencent/rapidjson.git third_party/rapidjson
git submodule add --depth 1 -b master https://github.com/leethomason/tinyxml2.git third_party/tinyxml2
git submodule add --depth 1 -b master https://github.com/skywind3000/kcp.git third_party/kcp
git submodule add --depth 1 -b develop https://github.com/madler/zlib.git third_party/zlib
```

本项目实际选择以 `.gitmodules` 为准。`--depth 1` 只下载浅历史以节省磁盘，源码仍完整。
需要历史记录时执行 `git -C third_party/yaml-cpp fetch --unshallow`。

首次获取整个项目：

```bash
git clone --recurse-submodules git@github.com:narcissus01/UReactor.git
# 已 clone 但缺少子模块源码时：
git submodule update --init --recursive
git submodule status
```

这些命令恢复主项目记录的提交；子模块显示 detached HEAD 通常是正常状态。
浅克隆旧提交不可达时，需要 fetch 补充历史，不要随意切换最新分支替代锁定版本。

## 3. CMake 接入结构

根 CMake 用 `UREACTOR_BUILD_THIRD_PARTY` 控制是否进入 `third_party/CMakeLists.txt`。
关闭时完全不需要配置第三方库；开启时使用源码构建，不把文件安装到系统目录。

| 库 | 提供给项目使用的目标 | 接入方式 |
|---|---|---|
| yaml-cpp | `yaml-cpp::yaml-cpp` | 上游 add_subdirectory |
| Abseil | `absl::strings` 等按需选用 | 上游 add_subdirectory |
| Protobuf | `protobuf::libprotobuf`、`protobuf::protoc` | 上游 add_subdirectory |
| RapidJSON | `UReactor::rapidjson` | 本地 INTERFACE 目标，仅头文件 |
| TinyXML2 | `tinyxml2::tinyxml2` | 上游 add_subdirectory |
| KCP | `UReactor::kcp` | 本地 STATIC 目标编译 ikcp.c |
| zlib | `zlibstatic` | 上游 add_subdirectory |

`add_subdirectory(... EXCLUDE_FROM_ALL SYSTEM)` 的含义：

- 读取上游 CMake 并注册目标；不把未使用的库默认全部编译。
- 某个程序链接该库时，其依赖仍会自动构建。
- SYSTEM 将第三方头文件视为系统头文件，减少业务编译中的第三方告警干扰。

目标式链接会传播需要的头文件路径、编译宏与传递依赖，通常不需要全局 `include_directories` / `link_directories`：

```cmake
target_link_libraries(my_app PRIVATE
    UReactor::ureactor
    yaml-cpp::yaml-cpp
    UReactor::rapidjson)
```

PRIVATE 是本目标使用；PUBLIC 是本目标和使用它的下游都需要；INTERFACE 是只向下游传播。
目前 UReactor 内核尚未调用这些库，所以没有强制把七个库链接进内核。
如果未来公开头文件暴露某库类型，还需同步完善 UReactor 的安装导出和 find_dependency。

Abseil 必须先于 Protobuf 添加，让 Protobuf 复用同一套 `absl::` 目标。
本项目禁止 Protobuf 配置时自动联网下载缺失依赖，缺失时直接报错。
Protobuf 的可选 zlib/gzip 集成暂关闭；zlib 自身独立可用，避免误链接机器上的系统版本。
上游分支并不保证任意版本组合兼容，当前提交组合需要通过集成测试后才能升级。

## 4. 配置、编译与验证

日常保持原来的命令：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
```

学习第三方库时使用独立 Release 目录，减少调试符号占用：

```bash
cmake -S . -B build-deps -DCMAKE_BUILD_TYPE=Release \
  -DUREACTOR_BUILD_THIRD_PARTY=ON -DUREACTOR_TEST_THIRD_PARTY=ON
cmake --build build-deps --target test_third_party --parallel 4
./build-deps/third_party/test_third_party
ctest --test-dir build-deps -R '^test_third_party$' --output-on-failure
```

集成测试实际执行 YAML/JSON/XML 解析、Abseil 字符串拼接、Protobuf 序列化往返、KCP 创建与发送排队、zlib 压缩解压。
它验证基本接入，不代表完整验证这些库的所有功能或 KCP 网络传输。
若内存不足，将 parallel 调低为 2；不要用无上限并行编译 Protobuf。

需要生成 `.pb.cc/.pb.h` 时可单独构建编译器：

```bash
cmake --build build-deps --target protoc --parallel 4
```

CMake 不会在每次 configure 时拉取子模块：缺失源码要先运行 Git 初始化命令。

## 5. 有意升级与提交

```bash
# 只升级一个库到 .gitmodules 里设置的分支
git submodule update --remote third_party/yaml-cpp
git diff --submodule=log
# 重新构建并运行测试后，再记录新提交
git add .gitmodules third_party/yaml-cpp
git commit -m "build: update yaml-cpp submodule"
git push origin main
```

初始化用 `update --init --recursive`；升级才使用 `update --remote`。不要将二者混淆。
对子模块直接改代码属于子模块自己的工作区，不会自动变成主项目普通文件的改动。
优先把适配写在外层 CMake，保持上游源码干净。

## 6. 网络排查

本机 HTTPS 不通但 GitHub SSH 已配置时，可对单次 Git 操作临时改用 SSH：

```bash
git -c url.git@github.com:.insteadOf=https://github.com/ \
  submodule update --init --recursive
```

这不会把 `.gitmodules` 中公开的 HTTPS 地址改成私有配置，不需要关闭 TLS/主机密钥验证。

## 7. 本次验证记录（2026-09-23）

WSL Ubuntu / GCC 13 / C++20 下，七库集成测试通过；原项目 Debug 构建及
test_formatter、test_log_buffer 回归测试通过。未进行 Windows 原生编译。
源码目录约 136 MB，浅克隆 Git 数据约 20 MB，Release 集成测试构建目录约 39 MB。
Protobuf 上游仍有 CMake 策略和弃用 API 警告，不是本次构建失败。
尚未构建 protoc 编译器或测试自定义 .proto 代码生成；这可作为下一节学习内容。

参考：[Git submodule](https://git-scm.com/docs/git-submodule)、
[CMake add_subdirectory](https://cmake.org/cmake/help/latest/command/add_subdirectory.html)、
[Protobuf Abseil 依赖说明](https://protobuf.dev/reference/cpp/abseil/)。


## 2026-10-08：配置系统接入 yaml-cpp

yaml-cpp 现在是 UReactor 配置系统的必需依赖，普通构建自动启用并随项目安装；其余六个库仍受 UREACTOR_BUILD_THIRD_PARTY 控制。公开配置头文件使用 YAML::Node，因此 ureactor 以 PUBLIC 链接 yaml-cpp::yaml-cpp。安装后的 UReactorConfig.cmake 使用 find_dependency(yaml-cpp CONFIG)，避免仅本机可编译、安装包无法链接。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ctest --test-dir build -E '^(test_macros|test_mutex)$' --output-on-failure
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 4
ctest --test-dir build-release -E '^test_mutex$' --output-on-failure
./build-release/tests/test_mutex 10000
```

Debug 中旧 test_macros 主动触发断言，改由 test_assertion_debug 子进程检查预期中止。test_mutex 必须传入操作次数，单独执行。

LoadFromYaml 现在返回 bool：任一配置转换失败或键非法时返回 false，已经更新的配置不会回滚。目录加载仅记录成功文件的时间戳，失败允许重试。文件按完整路径排序加载。递归深度限制为 64 层，循环 YAML 会抛出异常，目录加载入口捕获并报告异常。

ConfigVar 回调在锁外运行，允许重入；多线程通知可能并发或乱序，业务回调应自行同步。删除监听器不等待已获取的通知快照。PID 文件检查只判断 PID 是否存在，不能排除 PID 被复用。Windows 分支本次未实测。
