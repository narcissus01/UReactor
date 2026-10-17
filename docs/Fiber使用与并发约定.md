# Fiber 使用与并发约定

Fiber 是基于 Boost.Context 的有栈协程，不是 C++20 co_await 的无栈协程。

## 状态与唤醒

- 构造后为 INIT，resume 开始执行时变为 EXEC。
- YieldToReady 挂起为 READY，可在 resume 返回后再次调用 resume。
- YieldToHold(reason) 挂起为 WAITING，wake 成功将其改为 READY 并清除等待原因；重复 wake 返回 false。
- 回调完成为 TERM；回调抛出的普通异常被保存并进入 EXCEPT，通过 rethrowFailed 在调用方重新抛出。
- reset 仅允许在 TERM/EXCEPT、前一次 resume 已返回后执行；重置保留对象 ID，清空异常并回到 INIT。

resume 返回“本次挂起状态”；另一个线程可能已 wake，因此返回 WAITING 时 getState() 可以已经是 READY。

## 多线程和生命周期

同一对象的 resume/reset 通过 atomic_flag 排他，重叠调用直接抛 logic_error，不等待。该标记覆盖从进入 resume 到恢复调用栈并保存上下文的整个过程。因此不能只看到 TERM/READY 就立即跨线程 reset/resume，调度器必须等待上一次 resume 返回。wake 可在运行线程正在切出的短暂阶段调用，但下一次调度仍须等 resume 返回。

状态和等待原因的修改使用短临界区互斥锁协调，不持有 OS mutex 跨越协程切换，避免线程间迁移时在另一线程解锁。

允许在前一次 resume 返回后换线程恢复，但用户栈上不能持有要求原线程释放的资源，例如锁定的 std::mutex；线程局部变量也不属于协程私有数据。

wake 只唤醒当时处于 WAITING 的对象，不缓存提前到达的通知。IO/定时器调度器需要单独设计事件登记与挂起握手，避免“事件先到、随后才挂起”导致丢失通知。这里只提供上下文和状态管理，不实现调度器。

shared_ptr 管理的对象在 resume/reset 内临时保活。调用者仍须保证发起成员调用时对象有效；栈对象必须由外部保证不与执行并发析构。析构不会尝试替代调度器等待另一个线程。

不要把 GetThis() 的强引用长期留在挂起栈上：对象持有栈、栈再持有对象会形成循环。需要跨挂起保存身份时使用裸指针（由调度器保证存活）或 weak_ptr。

销毁挂起 Fiber 会触发 Boost forced_unwind，执行局部对象析构。清理期间 Impl 仍然存活且 Current() 指向该 Fiber。用户代码的 catch(...) 不得吞掉 forced_unwind；局部析构不得再次 Yield。

日志使用的 ureactor::GetFiberId() 已转发 Fiber::GetFiberId()，主调用栈返回 0。

## 验证命令

```bash
cd /mnt/d/CODE/UReactor
cmake --build build --parallel 4
ctest --test-dir build -E '^(test_macros|test_mutex)$' --output-on-failure
cmake --build build-release --parallel 4
ctest --test-dir build-release -E '^test_mutex$' --output-on-failure
```

测试包括 INIT/READY/WAITING/TERM/EXCEPT、两种栈、空回调与非法栈尺寸、异常重抛和 reset、嵌套 Fiber、析构时身份和 RAII、并发 wake、并发 resume/reset 拒绝、顺序跨线程迁移、释放最后一个外部 shared_ptr 后保活。并发测试不能证明任意调度下均无竞态，实际调度器仍需遵循以上约定。

运行验证环境为 WSL/Linux x86-64；未验证 Windows 原生或其他架构。

验证结果：2026-10-17，Debug 8/8、Release 9/9 回归通过。Release 的旧堆栈测试曾等待较久，本次通过 DEBUGINFOD_URLS= 临时关闭在线符号查找后全部完成；未修改系统环境配置。可用命令：

```bash
DEBUGINFOD_URLS= ctest --test-dir build-release -E '^test_mutex$' --timeout 30 --output-on-failure
```