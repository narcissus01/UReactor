#include <atomic>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <boost/context/detail/exception.hpp>
#include <boost/context/fiber.hpp>
#include <boost/context/fixedsize_stack.hpp>
#include <boost/context/protected_fixedsize_stack.hpp>
#include <boost/context/stack_traits.hpp>

#include <ureactor/fiber.h>


namespace ureactor {
namespace {

std::atomic<u64> g_nextFiberId{ 0 };
std::atomic<u64> g_FiberCount{ 0 };

thread_local Fiber* g_currentFiber = nullptr;

class CurrentFiberGuard final
{
public:
	explicit CurrentFiberGuard(Fiber* current) noexcept
		: m_previous(g_currentFiber)
	{
		g_currentFiber = current;
	}

	~CurrentFiberGuard() noexcept
	{
		g_currentFiber = m_previous;
	}

	CurrentFiberGuard(const CurrentFiberGuard&) = delete;
	CurrentFiberGuard& operator=(const CurrentFiberGuard&) = delete;
private:
	Fiber* m_previous;
};

// 不持有 OS 互斥锁跨越上下文切换；并发 resume/reset 立即拒绝。
class OperationGuard final {
public:
    explicit OperationGuard(std::atomic_flag& flag) : flag_(flag) {
        if (flag_.test_and_set(std::memory_order_acquire))
            throw std::logic_error("resume/reset already active on this fiber");
    }
    ~OperationGuard() { flag_.clear(std::memory_order_release); }
    OperationGuard(const OperationGuard&) = delete;
    OperationGuard& operator=(const OperationGuard&) = delete;
private:
    std::atomic_flag& flag_;
};

void ValidateStackSize(std::size_t stack_size)
{
	const std::size_t minimum = boost::context::stack_traits::minimum_size();
	if (stack_size < minimum)
	{
		throw std::invalid_argument("fiber stack size is smaller than Boost.Context minimum");
	}

	if (!boost::context::stack_traits::is_unbounded() && stack_size > boost::context::stack_traits::maximum_size())
	{
		throw std::invalid_argument("fiber stack size exceeds Boost.Context maximum");
	}
}

} //namespace

struct Fiber::Impl
{
	Impl(u64 fiber_id, Options fiber_options)
		: id(fiber_id)
		, options(fiber_options)
	{}

	u64 id;
	Options options;
	Callback callback;
	std::atomic<State> state{ State::INIT };
	std::atomic<State> suspendState{ State::INIT };
	std::atomic<WaitReason> waitReason{ WaitReason::None };
	std::atomic_flag operating = ATOMIC_FLAG_INIT;
	mutable std::mutex stateMutex;
	mutable std::mutex exceptionMutex;
	std::exception_ptr exception;
	boost::context::fiber caller;
	boost::context::fiber context;
};

Fiber::Fiber(Callback callback)
	: Fiber(std::move(callback), Options{})
{}

Fiber::Fiber(Callback callback, Options options)
	: m_impl(std::make_unique<Impl>(g_nextFiberId.fetch_add(1, std::memory_order_relaxed) + 1, options))
{
	ValidateStackSize(options.stack_size);
	initializeContext(std::move(callback));
	g_FiberCount.fetch_add(1, std::memory_order_relaxed);
}

Fiber::~Fiber() noexcept
{
    if (g_currentFiber == this || m_impl->operating.test(std::memory_order_acquire))
        std::terminate();
    // forced_unwind 在这里执行挂起栈的析构，Impl 和 Current 身份必须仍有效。
    {
        CurrentFiberGuard current(this);
        m_impl->context = {};
    }
    g_FiberCount.fetch_sub(1, std::memory_order_relaxed);
}

void Fiber::initializeContext(Callback callback)
{
	if (!callback)
	{
		throw std::invalid_argument("fiber callback must not be empty");
	}

	auto entry = [this](boost::context::fiber&& caller) -> boost::context::fiber
		{
			m_impl->caller = std::move(caller);
			run();
			return std::move(m_impl->caller);
		};

	boost::context::fiber new_context;
	if (m_impl->options.guardPage)
	{
		new_context = boost::context::fiber(std::allocator_arg, boost::context::protected_fixedsize_stack(m_impl->options.stack_size), std::move(entry));
	}

	else
	{
		new_context = boost::context::fiber(std::allocator_arg, boost::context::fixedsize_stack(m_impl->options.stack_size), std::move(entry));
	}
	{
		std::lock_guard lock(m_impl->exceptionMutex);
		m_impl->exception = nullptr;
	}
	m_impl->callback = std::move(callback);
	m_impl->caller = {};
	m_impl->context = std::move(new_context);
	m_impl->waitReason.store(WaitReason::None, std::memory_order_relaxed);
	m_impl->suspendState.store(State::INIT, std::memory_order_relaxed);
	m_impl->state.store(State::INIT, std::memory_order_relaxed);
}

Fiber::State Fiber::resume()
{
    // 调用者即使在回调中释放最后一个外部 shared_ptr，本次执行仍保持存活。
    const auto keep_alive = weak_from_this().lock();
    OperationGuard operation(m_impl->operating);
    {
        std::lock_guard lock(m_impl->stateMutex);
        const auto state = m_impl->state.load(std::memory_order_relaxed);
        if (state != State::INIT && state != State::READY)
            throw std::logic_error("only INIT or READY fiber can be resumed");
        if (!m_impl->context)
            throw std::logic_error("fiber has no resumable context");
        m_impl->state.store(State::EXEC, std::memory_order_release);
    }
    CurrentFiberGuard current(this);
    m_impl->context = std::move(m_impl->context).resume();
    // wake 可能已将 WAITING 改为 READY，返回值仍表示此次实际挂起的原因。
    return m_impl->suspendState.load(std::memory_order_acquire);
}

void Fiber::yield(State target, WaitReason reason)
{
    {
        std::lock_guard lock(m_impl->stateMutex);
        if (m_impl->state.load(std::memory_order_relaxed) != State::EXEC || !m_impl->caller)
            throw std::logic_error("only running fiber with a caller can yield");
        m_impl->waitReason.store(reason, std::memory_order_relaxed);
        m_impl->suspendState.store(target, std::memory_order_release);
        m_impl->state.store(target, std::memory_order_release);
    }
    m_impl->caller = std::move(m_impl->caller).resume();
}

void Fiber::reset(Callback callback)
{
    const auto keep_alive = weak_from_this().lock();
    OperationGuard operation(m_impl->operating);
	const State state = m_impl->state.load(std::memory_order_acquire);
	if (state != State::TERM && state != State::EXCEPT)
	{
		throw std::logic_error("fiber can only be reset after TERM or EXCEPT");
	}
	if (g_currentFiber == this)
	{
		throw std::logic_error("running fiber can not reset itself");
	}
	initializeContext(std::move(callback));
}

bool Fiber::wake() noexcept
{
    std::lock_guard lock(m_impl->stateMutex);
    if (m_impl->state.load(std::memory_order_relaxed) != State::WAITING) return false;
    m_impl->waitReason.store(WaitReason::None, std::memory_order_relaxed);
    m_impl->state.store(State::READY, std::memory_order_release);
    return true;
}

u64 Fiber::getId() const noexcept
{
	return m_impl->id;
}

Fiber::State Fiber::getState() const noexcept
{
	return m_impl->state.load(std::memory_order_acquire);
}

Fiber::WaitReason Fiber::getWaitReason() const noexcept
{
	std::lock_guard lock(m_impl->stateMutex);
	return m_impl->waitReason.load(std::memory_order_relaxed);
}

std::size_t Fiber::getStackSize() const noexcept
{
	return m_impl->options.stack_size;
}

bool Fiber::hasGuardPage() const noexcept
{
	return m_impl->options.guardPage;
}

std::exception_ptr Fiber::getException() const noexcept
{
	std::lock_guard lock(m_impl->exceptionMutex);
	return m_impl->exception;
}

void Fiber::rethrowFailed() const
{
	if (const std::exception_ptr exception = getException())
	{
		std::rethrow_exception(exception);
	}
}

Fiber* Fiber::Current() noexcept
{
	return g_currentFiber;
}

Fiber::ptr Fiber::GetThis()
{
	Fiber* current = Current();
	if (current == nullptr)
	{
		return nullptr;
	}
	ptr owner = current->weak_from_this().lock();
	if (!owner)
	{
		throw std::logic_error("current fiber is not owned by shared_ptr");
	}
	return owner;
}

u64 Fiber::GetFiberId() noexcept
{
	return g_currentFiber == nullptr ? 0 : g_currentFiber->getId();
}

u64 Fiber::TotalFibers() noexcept
{
	return g_FiberCount.load(std::memory_order_relaxed);
}

void Fiber::YieldToReady()
{
	if (g_currentFiber == nullptr)
	{
		throw std::logic_error("YieldToReady must be called inside a fiber");
	}
	g_currentFiber->yield(State::READY, WaitReason::None);
}

void Fiber::YieldToHold(WaitReason reason)
{
	if (g_currentFiber == nullptr)
	{
		throw std::logic_error("YieldToHold must be inside a fiber");
	}
	if (reason == WaitReason::None)
	{
		throw std::invalid_argument("waiting fiber requires a wait reason");
	}
	g_currentFiber->yield(State::WAITING, reason);
}

void Fiber::run()
{
	try
	{
		m_impl->callback();
		m_impl->callback = nullptr;
        std::lock_guard state_lock(m_impl->stateMutex);
		m_impl->waitReason.store(WaitReason::None, std::memory_order_relaxed);
		m_impl->suspendState.store(State::TERM, std::memory_order_relaxed);
		m_impl->state.store(State::TERM, std::memory_order_release);
	}
	catch (const boost::context::detail::forced_unwind&)
	{
		throw;
	}
	catch (...)
	{
		{
			std::lock_guard lock(m_impl->exceptionMutex);
			m_impl->exception = std::current_exception();
		}
		m_impl->callback = nullptr;
        std::lock_guard state_lock(m_impl->stateMutex);
		m_impl->waitReason.store(WaitReason::None, std::memory_order_relaxed);
		m_impl->suspendState.store(State::EXCEPT, std::memory_order_relaxed);
		m_impl->state.store(State::EXCEPT, std::memory_order_release);
	}
}

std::string_view Fiber::StateName(State state) noexcept
{
	switch (state)
	{
	#define XX(name) case State::name: return #name;
			 XX(INIT)
			 XX(READY)
			 XX(EXEC)
			 XX(WAITING)
			 XX(TERM)
			 XX(EXCEPT)
#undef XX
	}
	return "UNKNOWN";
}

std::string_view Fiber::WaitReasonName(WaitReason reason) noexcept
{
	switch (reason)
	{
	case WaitReason::None:
		return "NONE";
	case WaitReason::IO:
		return "IO";
	case WaitReason::Timer:
		return "Timer";
	case WaitReason::Semaphore:
		return "SEMAPHORE";
	case WaitReason::External:
		return "EXTERNAL";
	}
	return "UNKNOWN";
}

} // namespace ureactor
