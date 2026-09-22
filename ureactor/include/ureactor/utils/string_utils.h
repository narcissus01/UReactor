#ifndef UREACTOR_INCLUDE_UREACTOR_UTILS_H
#define UREACTOR_INCLUDE_UREACTOR_UTILS_H

#include<cstring>
#include<cstdarg>
#include<string_view>
#include<string>

#include<ureactor/export.h>

namespace ureactor{

class UREACTOR_API StringUtils final
{
public:
    StringUtils() = delete;
    [[nodiscard]] static std::string WstringToString(std::wstring_view text) noexcept;
    [[nodiscard]] static std::wstring StringToWstring(std::string_view text) noexcept;
};


}

#endif