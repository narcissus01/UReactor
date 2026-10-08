#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <ureactor/macros.h>
#include <ureactor/utils/string_utils.h>
#include <ureactor/utils/files_utils.h>


namespace ureactor{

bool FSUtils::MKdir(std::string_view dirname) noexcept
{
    try {
        if (dirname.empty()) return false;
        const std::filesystem::path path(dirname);
        std::error_code error;
        std::filesystem::create_directories(path, error);
        return !error && std::filesystem::is_directory(path, error) && !error;
    } catch (...) { return false; }
}

bool FSUtils::Unlink(std::string_view file_name, bool exist) noexcept
{
    // exist=true 要求目标原本存在；不递归删除目录。
    try {
        if (file_name.empty()) return false;
        std::error_code error;
        const bool removed = std::filesystem::remove(std::filesystem::path(file_name), error);
        return !error && (removed || !exist);
    } catch (...) { return false; }
}

bool FSUtils::IsRunningPidfile(std::string_view pidfile) noexcept
{
    try {
        std::ifstream input{std::filesystem::path(pidfile)};
        std::string line;
        if (!std::getline(input, line)) return false;
        const auto text = StringUtils::Trim(std::string_view(line));
        if (text.empty()) return false;
        unsigned long pid = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), pid);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || pid == 0)
            return false;
#if defined(_WIN32)
        if (pid > std::numeric_limits<DWORD>::max()) return false;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
        if (!process) return GetLastError() == ERROR_ACCESS_DENIED;
        DWORD status = 0;
        const bool running = GetExitCodeProcess(process, &status) && status == STILL_ACTIVE;
        CloseHandle(process);
        return running;
#else
        if (pid > static_cast<unsigned long>(std::numeric_limits<pid_t>::max())) return false;
        return kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM;
#endif
    } catch (...) { return false; }
}

void FSUtils::ListAllFile(std::vector<std::string>& files, std::string_view path, std::string_view suffix)
{
    if(path.empty())
    {
        return;
    }

    const std::size_t first_new_file = files.size();
    try
    {
        const std::filesystem::path root(path);
        if(!std::filesystem::is_directory(root))
        {
            return;
        }

        const auto options = std::filesystem::directory_options::skip_permission_denied;
        for(const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root, options))
        {
            std::error_code error;
            if (!entry.is_regular_file(error) || error)
            {
                continue;
            }

            std::string file_name = entry.path().lexically_normal().string();
            if(suffix.empty() || file_name.ends_with(suffix))
            {
                files.push_back(std::move(file_name));
            }
        }
    }
    catch(const std::filesystem::filesystem_error&)
    {}

    std::sort(files.begin() + static_cast<std::ptrdiff_t>(first_new_file), files.end());

}

}
