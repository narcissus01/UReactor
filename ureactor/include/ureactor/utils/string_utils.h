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

    // 字符串字面量直接选用视图重载，避免 string/string_view 转换歧义。
    [[nodiscard]] static std::string_view Trim(const char* text, std::string_view delimiters = " \t\n\r\f\v") noexcept { return Trim(std::string_view(text), delimiters); }
    [[nodiscard]] static std::string_view TrimLeft(const char* text, std::string_view delimiters = " \t\n\r\f\v") noexcept { return TrimLeft(std::string_view(text), delimiters); }
    [[nodiscard]] static std::string_view TrimRight(const char* text, std::string_view delimiters = " \t\n\r\f\v") noexcept { return TrimRight(std::string_view(text), delimiters); }

    inline static constexpr std::string_view ASCII_WHITESPACE{" \t\n\r\f\v"};

    [[nodiscard]] static std::string Trim(const std::string& text, std::string_view delimiters = ASCII_WHITESPACE);

    [[nodiscard]] static std::string_view Trim(std::string_view text, std::string_view delimiters = ASCII_WHITESPACE) noexcept;

    [[nodiscard]] static std::string TrimLeft(const std::string& text, std::string_view delimiters = ASCII_WHITESPACE);

    [[nodiscard]] static std::string_view TrimLeft(std::string_view text, std::string_view delimiters = ASCII_WHITESPACE) noexcept;

    [[nodiscard]] static std::string TrimRight(const std::string& text, std::string_view delimiters = ASCII_WHITESPACE);

    [[nodiscard]] static std::string_view TrimRight(std::string_view text, std::string_view delimiters = ASCII_WHITESPACE) noexcept;
};


}

#endif
