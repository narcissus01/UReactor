#include <ureactor/fiber.h>
#include <ureactor/utils/system_utils.h>
#include <atomic>
#include <iostream>
#include <thread>
#include <stdexcept>

using ureactor::Fiber;
using S = Fiber::State;
#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error(#__VA_ARGS__); } while(0)
template<class F> void Reject(F&& f) {
    bool rejected = false;
    try { f(); } catch (const std::logic_error&) { rejected = true; }
    CHECK(rejected);
}
int main() try {
    const auto count = Fiber::TotalFibers();
    CHECK(!Fiber::Current() && !Fiber::GetThis() && ureactor::GetFiberId()==0);
    Reject([] { Fiber::YieldToReady(); });
    Reject([] { Fiber f(Fiber::Callback{}); });
    Reject([] { Fiber f([]{}, {1, true}); });
    for (bool guard : {true, false}) {
        int stage = 0;
        Fiber f([&] {
            CHECK(Fiber::Current()==&f);
            CHECK(ureactor::GetFiberId()==f.getId());
            Reject([&] { f.resume(); });
            Reject([&] { f.reset([]{}); });
            Reject([] { (void)Fiber::GetThis(); });
            stage=1; Fiber::YieldToReady();
            stage=2; Fiber::YieldToHold(Fiber::WaitReason::IO);
            CHECK(f.getWaitReason()==Fiber::WaitReason::None);
            stage=3;
        }, {Fiber::DEFAULT_STACK_SIZE,guard});
        CHECK(f.hasGuardPage()==guard && f.getState()==S::INIT);
        Reject([&] { f.reset([]{}); });
        CHECK(f.resume()==S::READY && stage==1 && !Fiber::Current());
        CHECK(f.resume()==S::WAITING && stage==2);
        CHECK(f.getWaitReason()==Fiber::WaitReason::IO);
        Reject([&] { f.resume(); });
        std::atomic<int> wakes{0};
        std::thread a([&] { if(f.wake()) ++wakes; });
        std::thread b([&] { if(f.wake()) ++wakes; });
        a.join(); b.join(); CHECK(wakes==1);
        CHECK(f.resume()==S::TERM && stage==3);
        Reject([&] { f.resume(); });
        f.reset([] { throw std::runtime_error("failure"); });
        CHECK(f.resume()==S::EXCEPT && f.getException());
        bool caught=false;
        try { f.rethrowFailed(); } catch(const std::runtime_error&) { caught=true; }
        CHECK(caught);
        f.reset([]{}); CHECK(!f.getException() && f.resume()==S::TERM);
    }
    bool unwound=false, identity=false;
    {
        Fiber f([&] {
            struct Guard {
                Fiber* self; bool& done; bool& identity;
                ~Guard() { done=true; identity=Fiber::Current()==self && ureactor::GetFiberId()==self->getId(); }
            } guard{Fiber::Current(),unwound,identity};
            Fiber::YieldToHold();
        });
        CHECK(f.resume()==S::WAITING);
    }
    CHECK(unwound && identity && !Fiber::Current());
    {
        // 只在前一次 resume 返回后跨线程迁移，不能并行执行同一上下文。
        std::atomic<int> steps{0};
        Fiber migrated([&] {
            ++steps; Fiber::YieldToReady();
            CHECK(Fiber::Current()==&migrated);
            ++steps;
        });
        S first=S::INIT, second=S::INIT;
        std::thread a([&] { first=migrated.resume(); }); a.join();
        std::thread b([&] { second=migrated.resume(); }); b.join();
        CHECK(first==S::READY && second==S::TERM && steps==2);
    }
    {
        Fiber outer([] {
            auto* parent=Fiber::Current();
            Fiber inner([] { Fiber::YieldToReady(); });
            CHECK(inner.resume()==S::READY && Fiber::Current()==parent);
            CHECK(inner.resume()==S::TERM && Fiber::Current()==parent);
        });
        CHECK(outer.resume()==S::TERM);
    }
    {
        std::atomic<bool> entered=false, finish=false;
        Fiber f([&] { entered.store(true); entered.notify_all(); while(!finish.load()) finish.wait(false); });
        std::thread runner([&] { f.resume(); });
        while(!entered.load()) entered.wait(false);
        // 先结束线程再检查，失败时也不遗留 joinable thread。
        bool resumeRejected=false, resetRejected=false;
        try { f.resume(); } catch(const std::logic_error&) { resumeRejected=true; }
        try { f.reset([]{}); } catch(const std::logic_error&) { resetRejected=true; }
        finish.store(true); finish.notify_all(); runner.join();
        CHECK(resumeRejected && resetRejected && f.getState()==S::TERM);
    }
    {
        Fiber::ptr owner;
        owner=std::make_shared<Fiber>([&] { CHECK(Fiber::GetThis()==owner); owner.reset(); });
        CHECK(owner->resume()==S::TERM && !owner);
    }
    CHECK(Fiber::TotalFibers()==count && !Fiber::Current());
    std::cout << "Fiber state, wake, exception, reset, lifetime and concurrency checks passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
