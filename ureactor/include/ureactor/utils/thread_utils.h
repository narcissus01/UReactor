#ifndef UREACTOR_INCLUDE_UREACTOR_UTILS_THREAD_UTILS_H
#define UREACTOR_INCLUDE_UREACTOR_UTILS_THREAD_UTILS_H

#include<string>
#include<string_view>
#include<ureactor/export.h>
#include<ureactor/macros.h>

namespace ureactor{

[[nodiscard]] UREACTOR_API u64 GetThreadId() noexcept;

UREACTOR_API void SetThreadName(std::string name);

[[nodiscard]] UREACTOR_API std::string_view GetThreadName() noexcept;

}

#endif