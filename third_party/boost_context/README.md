# Boost.Context 1.92.0 精简嵌入

来源：用户提供的 `D:/boost_1_92_0.tar.gz`。源码内 BOOST_VERSION=109200。
压缩包 SHA-256：`c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce`。
这是本机文件指纹，未与下载站签名独立比对。

## 保留内容

共 845 个上游文件、3,663,806 字节（约 3.49 MiB），含 725 个头文件；不含本目录包装 CMake、说明和 manifest 的大小。

- Context 的公开头文件、源码、各平台汇编及构建脚本。
- 上游 CMake 声明的递归依赖：assert、config、core、integer、mp11、pool、predef、smart_ptr、throw_exception、type_traits、winapi。
- 依赖模块头文件目录完整保留，避免 config/predef 的条件和宏生成 include 被漏掉；继续追踪字面量 include，保留根目录兼容头文件。
- Boost Software License 1.0；上游源码和各模块 CMake 保持原样。
- 不携带完整 Boost、文档、上游测试和其他二进制库，不依赖机器上的系统 Boost。

`manifest.json` 记录模块依赖和每个原始文件的 SHA-256。外层 CMakeLists.txt 是 UReactor 的适配层：使用共享 include 目录、静态构建并加入 UReactor 安装导出。只有 Context 生成二进制库，其余依赖是 INTERFACE 头文件目标。

## 使用

项目已自动构建并通过 ureactor 的 PUBLIC 依赖传播 Boost::context，无须打开 UREACTOR_BUILD_THIRD_PARTY。

```cmake
target_link_libraries(your_target PRIVATE UReactor::ureactor)
# 独立使用上下文功能时：
target_link_libraries(your_target PRIVATE Boost::context)
```

```bash
cd /mnt/d/CODE/UReactor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target test_context --parallel 4
ctest --test-dir build -R '^test_context$' --output-on-failure
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target test_context --parallel 4
ctest --test-dir build-release -R '^test_context$' --output-on-failure
```

当前 WSL 为 x86_64/sysv/elf/gas，采用 fcontext 汇编后端。测试覆盖 fiber 的延迟启动、挂起和恢复、正常返回、挂起时销毁的 RAII 展开、带保护页的栈、池化栈以及 continuation/callcc。其他平台源码保留，但没有宣称已在 Windows 原生、ARM 或其他后端实测。

注意：boost::context::fiber 是有栈执行上下文，不是 Boost.Fiber 调度库；这里也没有引入 Boost.Coroutine2。C++20 co_await 使用的无栈协程机制与 Context 不同，后续需要按服务器架构选择或封装。

## 重复提取与升级

Python 3.11+：

```powershell
python tools/vendor_boost_context.py D:/boost_1_92_0.tar.gz --output D:/WSL/setup/boost-context-review
```

脚本只读取压缩包、拒绝覆盖已有输出目录，单次流式扫描 gzip；不需要完整解压到硬盘。输出为上游文件和 manifest，不含本项目的包装 CMake/说明。升级先提取到新目录，比较 manifest 与上游 CMake 依赖，再同步适配层版本并重新运行测试。不要直接覆盖现有目录。

本地原压缩包保持不变，编译产物仍位于 D 盘项目 build/build-release 中。
