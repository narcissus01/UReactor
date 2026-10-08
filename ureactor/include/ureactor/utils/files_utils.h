#ifndef UREACTOR_INCLUDE_UREACTOR_UTILS_FILES_UTILS_H
#define UREACTOR_INCLUDE_UREACTOR_UTILS_FILES_UTILS_H

#include <string>
#include <string_view>
#include <vector>

#include <ureactor/export.h>

namespace ureactor{

class UREACTOR_API FSUtils final
{
public:
    FSUtils() = delete;
    static void ListAllFile(std::vector<std::string>& files, std::string_view path, std::string_view suffix = {});
    // 递归创建目录；已存在且是目录也返回 true。
    [[nodiscard]] static bool MKdir(std::string_view dirname) noexcept;
    // 检查正整数 PID 当前是否存在，不保证它仍是最初写入文件的进程。
    [[nodiscard]] static bool IsRunningPidfile(std::string_view pidfile) noexcept;
    // 非递归删除；exist=true 时，目标不存在返回 false。
    [[nodiscard]] static bool Unlink(std::string_view file_name, bool exist = false) noexcept;
};

}

#endif
