#ifndef UREACTOR_INCLUDE_MUTEX_H
#define UREACTOR_INCLUDE_MUTEX_H

#include<atomic>
#include<mutex>

#if defined(__x86_64__) || defined(__i386__)
#include<immintrin.h>
#elif defined(__aarch64__) || defined(__arm__)
#include<arm_acle.h>
#endif

#if defined(__linux__) && defined(__GLIBC__)
#include<pthread.h>
#endif

inline void cpu_relax() noexcept
{
#if defined(__x86_64__) || defined(__i386__)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __yield();
#endif
}

namespace ureactor
{

#if defined(__linux__) && defined(__GLIBC__)

class SpinLock
{
public:
    SpinLock() noexcept
    {
        pthread_spin_init(&m_mutex, PTHREAD_PROCESS_PRIVATE);
    }

    ~SpinLock() noexcept
    {
        pthread_spin_destroy(&m_mutex);
    }

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    void lock() noexcept
    {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() noexcept
    {
        pthread_spin_unlock(&m_mutex);
    }

private:
    pthread_spinlock_t m_mutex{};
};

#else

class SpinLock
{
public:
    using Lock = std::lock_guard<SpinLock>;

    SpinLock() noexcept =default;
    ~SpinLock() noexcept =default;

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    void lock() noexcept
    {
        while (m_mutex.test_and_set(std::memory_order_acquire)) {
            while (m_mutex.test(std::memory_order_relaxed)) {
                cpu_relax();
            }
        }
    }

    [[nodiscard]] bool try_lock() noexcept
    {
        return !m_mutex.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept
    {
        m_mutex.clear(std::memory_order_release);
    }

private:
    std::atomic_flag m_mutex = ATOMIC_FLAG_INIT;
};

#endif

}

#endif
