#ifndef UREACTOR_INCLUDE_UREACTOR_UTILS_SYSTEM_UTILS_H
#define UREACTOR_INCLUDE_UREACTOR_UTILS_SYSTEM_UTILS_H

#include<chrono>
#include<string>
#include<ureactor/types.h>

#include<ureactor/export.h>

namespace ureactor{

[[nodiscard]] UREACTOR_API std::chrono::steady_clock::duration GetElapsedTime() noexcept;

[[nodiscard]] UREACTOR_API u64 GetFiberId() noexcept;

// size 为最多捕获的帧数（包含被跳过的帧）；skip 为跳过的顶部帧数，负值按 0。
// 每行输出 prefix + #序号 + 函数名/地址，存在调试信息时附加 " at 文件:行号"。
// size <= 0 或 skip 超出捕获帧数时返回空字符串。可能因内存分配失败抛异常。
// 用于普通调用路径，不可在异步信号处理函数中调用。
[[nodiscard]] UREACTOR_API std::string GetBacktrace(int size = 20, int skip = 1, const std::string prefix = "");

}

#endif
