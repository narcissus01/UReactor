#ifndef UREACTOR_INCLUDE_UREACTOR_UTILS_SYSTEM_UTILS_H
#define UREACTOR_INCLUDE_UREACTOR_UTILS_SYSTEM_UTILS_H

#include<chrono>

#include<ureactor/macros.h>
#include<ureactor/export.h>

namespace ureactor{

[[nodiscard]] UREACTOR_API std::chrono::steady_clock::duration GetElapsedTime() noexcept;

[[nodiscard]] UREACTOR_API u64 GetFiberId() noexcept;

}

#endif