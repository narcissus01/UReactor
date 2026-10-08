#include <ureactor/utils/system_utils.h>
#include <iostream>

int main()
{
    if (!ureactor::GetBacktrace(0).empty()) return 1;
    if (!ureactor::GetBacktrace(-1).empty()) return 2;
    if (!ureactor::GetBacktrace(4, 100).empty()) return 3;
    const auto trace = ureactor::GetBacktrace(8, -1, "trace: ");
    if (!trace.starts_with("trace: #0 ")) return 4;
    std::cout << trace;
    // 本项目 Linux Debug 带 -g3，应能恢复当前源码位置。
#if defined(__linux__) && !defined(NDEBUG)
    if (trace.find("test_backtrace.cpp:") == std::string::npos) return 5;
#endif
    return 0;
}
