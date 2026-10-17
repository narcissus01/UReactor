#include <boost/context/continuation.hpp>
#include <boost/context/fiber.hpp>
#include <boost/context/protected_fixedsize_stack.hpp>
#include <boost/context/pooled_fixedsize_stack.hpp>
#include <iostream>
#include <stdexcept>
#include <utility>

#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error(#__VA_ARGS__); } while (0)

int main() try {
    int stage = 0;
    bool destroyed = false;
    struct Guard { bool& value; ~Guard() { value = true; } };
    {
        boost::context::fiber task(std::allocator_arg,
            boost::context::protected_fixedsize_stack(128 * 1024),
            [&](boost::context::fiber&& caller) {
                Guard guard{destroyed};
                stage = 1;
                caller = std::move(caller).resume();
                stage = 2;
                return std::move(caller);
            });
        CHECK(stage == 0);
        task = std::move(task).resume();
        CHECK(stage == 1 && !destroyed && task);
        task = std::move(task).resume();
        CHECK(stage == 2 && destroyed && !task);
    }
    destroyed = false;
    {
        boost::context::fiber task(std::allocator_arg,
            boost::context::pooled_fixedsize_stack(128 * 1024),
            [&](boost::context::fiber&& caller) {
            Guard guard{destroyed};
            caller = std::move(caller).resume();
            return std::move(caller);
        });
        task = std::move(task).resume();
        CHECK(!destroyed);
        // 销毁仍挂起的上下文应展开其栈，调用局部对象析构函数。
    }
    CHECK(destroyed);
    int value = 0;
    auto continuation = boost::context::callcc([&](boost::context::continuation&& caller) {
        value = 10;
        caller = caller.resume();
        value += 5;
        return std::move(caller);
    });
    CHECK(value == 10);
    continuation = continuation.resume();
    CHECK(value == 15 && !continuation);
    std::cout << "Boost.Context: fiber, continuation and stack unwinding passed\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
