#ifndef UREACTOR_INCLUDE_UREACTOR_FIBER_H
#define UREACTOR_INCLUDE_UREACTOR_FIBER_H

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string_view>

#include <ureactor/export.h>
#include <ureactor/types.h>

namespace ureactor {

class UREACTOR_API Fiber final : public std::enable_shared_from_this<Fiber>
{
public:
	using ptr = std::shared_ptr<Fiber>;
	using Callback = std::function<void()>;

	enum class State : u8
	{
		INIT,
		READY,
		EXEC,
		WAITING,
		TERM,
		EXCEPT,
	};

	enum class WaitReason : u8
	{
		None,
		IO,
		Timer,
		Semaphore,
		External,
	};

	static constexpr std::size_t DEFAULT_STACK_SIZE = 128U * 1024U;
	struct Options
	{
		std::size_t stack_size = DEFAULT_STACK_SIZE;
		bool guardPage = true;
	};

	explicit Fiber(Callback callback);
	Fiber(Callback callback, Options options);

	~Fiber() noexcept;

	Fiber(const Fiber&) = delete;
	Fiber& operator=(const Fiber&) = delete;
	Fiber(Fiber&&) = delete;
	Fiber& operator=(Fiber&&) = delete;

    // 同一个对象的 resume/reset 不得重叠，重叠调用抛 logic_error。
    // 调度器必须等 resume 返回后再调度下一次；wake 可以跨线程调用。
    // 返回本次挂起状态，可能与随后被 wake 修改的 getState() 不同。
	State resume();

    // 仅在 TERM/EXCEPT 且上一次 resume 已返回时可重置。
	void reset(Callback callback);
	[[nodiscard]] bool wake() noexcept;
	[[nodiscard]] u64 getId() const noexcept;
	[[nodiscard]] State getState() const noexcept;
	[[nodiscard]] WaitReason getWaitReason() const noexcept;
	[[nodiscard]] std::size_t getStackSize() const noexcept;
	[[nodiscard]] bool hasGuardPage() const noexcept;
	[[nodiscard]] std::exception_ptr getException() const noexcept;

	void rethrowFailed() const;

	[[nodiscard]] static Fiber* Current() noexcept;
    // 要求对象由 shared_ptr 管理；不要将强引用长期保留在挂起栈上造成循环。
	[[nodiscard]] static ptr GetThis();
	[[nodiscard]] static u64 GetFiberId() noexcept;
	[[nodiscard]] static u64 TotalFibers() noexcept;

	static void YieldToReady();
	static void YieldToHold(WaitReason reason = WaitReason::External);

	[[nodiscard]] static std::string_view StateName(State state) noexcept;
	[[nodiscard]] static std::string_view WaitReasonName(WaitReason reason) noexcept;
private:
	void initializeContext(Callback callback);
	void run();
	void yield(State target, WaitReason reason);
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

};

}

#endif
