#ifndef UREACTOR_INCLUDE_UREACTOR_MACROS_H
#define UREACTOR_INCLUDE_UREACTOR_MACROS_H

#include <cassert>
#include <cstdio>
#include <sstream>

#include<ureactor/types.h>
#include<ureactor/utils/system_utils.h>

namespace ureactor::detail {

// 不依赖 Logger：日志器自身也使用断言，调用它会导致递归或重入锁。
// 每个线程独立防重入；嵌套失败只打印基本信息，不再格式化消息或解析堆栈。
inline thread_local bool reporting_assertion = false;

template<class WriteMessage>
void ReportAssertionFailure(const char* file, int line, const char* expression,
                            WriteMessage&& write_message) noexcept
{
    std::fprintf(stderr, "file: %s, line: %d, Assertion failed: %s\n",
                 file, line, expression);
    if (reporting_assertion) {
        std::fputs("[recursive assertion: details omitted]\n", stderr);
        std::fflush(stderr);
        return;
    }
    reporting_assertion = true;
    struct Reset {
        ~Reset() { reporting_assertion = false; }
    } reset;

    // 消息表达式也在 try 内求值，防止分配失败或用户 operator<< 抛出异常。
    try {
        std::ostringstream message;
        write_message(message);
        const auto text = message.str();
        std::fputs("Message: ", stderr);
        std::fwrite(text.data(), 1, text.size(), stderr);
        std::fputc('\n', stderr);
    } catch (...) {
        std::fputs("Message: [unavailable: formatting threw]\n", stderr);
    }
    try {
        const auto trace = GetBacktrace();
        std::fputs("Stacktrace:\n", stderr);
        std::fwrite(trace.data(), 1, trace.size(), stderr);
    } catch (...) {
        std::fputs("Stacktrace: [unavailable: capture threw]\n", stderr);
    }
    std::fflush(stderr);
}
} // namespace ureactor::detail

// Debug: print and abort. NDEBUG: print and let the caller handle failure.
// expr_text is a string, so the original condition is never evaluated here.
#define UREACTOR_DETAIL_FAILURE(expr_text, msg) \
    do { \
        ::ureactor::detail::ReportAssertionFailure(__FILE__, __LINE__, (expr_text), \
            [&](std::ostream& ureactor_message_stream) { ureactor_message_stream << (msg); }); \
        assert(false && "UReactor assertion failed; see diagnostic above"); \
    } while (0)

#define UREACTOR_ASSERT(x) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, ""); \
        } \
    } while (0)

#define UREACTOR_ASSERT_MSG(x, msg) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, msg); \
        } \
    } while (0)

#define ASSERT_RETVAL(x, val) ASSERT_RETVAL_MSG(x, val, "")
#define ASSERT_RETVAL_MSG(x, val, msg) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, msg); \
            return (val); \
        } \
    } while (0)

#define ASSERT_RETNONE(x) ASSERT_RETNONE_MSG(x, "")
#define ASSERT_RETNONE_MSG(x, msg) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, msg); \
            return; \
        } \
    } while (0)

#define ASSERT_NOEFFECT(x) UREACTOR_ASSERT(x)
#define ASSERT_NOEFFECT_MSG(x, msg) UREACTOR_ASSERT_MSG(x, msg)

// Do not wrap these in do/while: break/continue must affect the caller's loop.
// The caller's semicolon completes the final expression statement.
#define ASSERT_CONTINUE(x) ASSERT_CONTINUE_MSG(x, "")
#define ASSERT_CONTINUE_MSG(x, msg) \
    if (!(x)) [[unlikely]] { \
        UREACTOR_DETAIL_FAILURE(#x, msg); \
        continue; \
    } else (void)0

#define ASSERT_BREAK(x) ASSERT_BREAK_MSG(x, "")
#define ASSERT_BREAK_MSG(x, msg) \
    if (!(x)) [[unlikely]] { \
        UREACTOR_DETAIL_FAILURE(#x, msg); \
        break; \
    } else (void)0



#endif
