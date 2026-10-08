// 两个独立可执行文件分别验证 Debug 中止和 Release 回退，与构建类型无关。
#ifdef UREACTOR_TEST_RELEASE
#ifndef NDEBUG
#define NDEBUG
#endif
#else
#undef NDEBUG
#endif
#include <ureactor/macros.h>
#include <stdexcept>

struct RecursiveMessage {};
std::ostream& operator<<(std::ostream& out, RecursiveMessage)
{
    UREACTOR_ASSERT_MSG(false, "nested diagnostic");
    return out << "outer diagnostic";
}

const char* ThrowingMessage() { throw std::runtime_error("message failed"); }

int ReturnFallback() noexcept
{
    ASSERT_RETVAL_MSG(false, 42, ThrowingMessage());
    return 0;
}

int main()
{
#ifdef UREACTOR_TEST_RELEASE
    int evaluations = 0;
    int messages = 0;
    UREACTOR_ASSERT_MSG(++evaluations == 1, ++messages);
    if (evaluations != 1 || messages != 0) return 1;
    UREACTOR_ASSERT_MSG(++evaluations == 0, ++messages);
    if (evaluations != 2 || messages != 1) return 2;
    if (ReturnFallback() != 42) return 3;
    UREACTOR_ASSERT_MSG(false, RecursiveMessage{});
    if (ureactor::detail::reporting_assertion) return 4;
    int sum = 0;
    for (int i = 0; i < 4; ++i) {
        ASSERT_CONTINUE(i != 1);
        sum += i;
    }
    if (sum != 5) return 5;
    int index = 0;
    for (; index < 4; ++index) {
        ASSERT_BREAK(index != 2);
    }
    if (index != 2) return 6;
    bool continued = false;
    [&] { ASSERT_RETNONE(false); continued = true; }();
    if (continued) return 7;
    std::puts("fallback checks passed");
    return 0;
#else
    // 没有创建 Logger，依然应输出表达式、消息、堆栈并中止。
    UREACTOR_ASSERT_MSG(false, "debug diagnostic marker");
    return 99;
#endif
}
