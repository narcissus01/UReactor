#ifndef UREACTOR_INCLUDE_UREACTOR_LOGGER_LOG_H
#define UREACTOR_INCLUDE_UREACTOR_LOGGER_LOG_H

#include<cstddef>
#include<iosfwd>
#include<string_view>

#include<ureactor/logger/appender.h>
#include<ureactor/logger/loglevel.h>
#include<ureactor/logger/logger.h>
#include<ureactor/macros.h>

namespace ureactor::detail{

class UREACTOR_API LogLine final
{
public:
    LogLine(Logger& logger, Loglevel::Level level, u32 line, std::string_view file_name);
    ~LogLine() noexcept;
    LogLine(const LogLine&) = delete;
    LogLine& operator=(const LogLine&) = delete;
    LogLine(LogLine&&) = delete;
    LogLine& operator=(LogLine&&) = delete;

    [[nodiscard]] std::ostream& stream() noexcept;

private:
    struct Impl;
    
    static constexpr std::size_t LOG_LINE_IMPL_SIZE = 1024;
    alignas(std::max_align_t) std::byte m_implStorage[LOG_LINE_IMPL_SIZE];

    [[nodiscard]] Impl& getImpl() noexcept;
};

UREACTOR_API void LogPrintf(Logger& logger, Loglevel::Level level, u32 line, std::string_view file_name, const char* format, ...);

}

#define UREACTOR_LOG_LEVEL(logger, level) \
    if (auto ureactor_log_logger = (logger); !ureactor_log_logger) {} \
    else if (const auto ureactor_log_level = (level); \
             !ureactor_log_logger->shouldLog(ureactor_log_level)) {} \
    else ureactor::detail::LogLine( \
        *ureactor_log_logger, ureactor_log_level, \
        __LINE__, __FILE__).stream()

#define UREACTOR_LOG_DEBUG(logger) \
    UREACTOR_LOG_LEVEL((logger), ureactor::Loglevel::Level::LOG_LV_DEBUG)
#define UREACTOR_LOG_INFO(logger) \
    UREACTOR_LOG_LEVEL((logger), ureactor::Loglevel::Level::LOG_LV_INFO)
#define UREACTOR_LOG_WARN(logger) \
    UREACTOR_LOG_LEVEL((logger), ureactor::Loglevel::Level::LOG_LV_WARN)
#define UREACTOR_LOG_ERROR(logger) \
    UREACTOR_LOG_LEVEL((logger), ureactor::Loglevel::Level::LOG_LV_ERROR)
#define UREACTOR_LOG_FATAL(logger) \
    UREACTOR_LOG_LEVEL((logger), ureactor::Loglevel::Level::LOG_LV_FATAL)

#define UREACTOR_LOG_FMT_LEVEL(logger, level, format, ...) \
    if (auto ureactor_log_logger = (logger); !ureactor_log_logger) {} \
    else if (const auto ureactor_log_level = (level); \
             !ureactor_log_logger->shouldLog(ureactor_log_level)) {} \
    else ureactor::detail::LogPrintf( \
        *ureactor_log_logger, ureactor_log_level, \
        __LINE__, __FILE__, (format) __VA_OPT__(,) __VA_ARGS__)

#define UREACTOR_LOG_FMT_DEBUG(logger, format, ...) \
    UREACTOR_LOG_FMT_LEVEL( \
        (logger), ureactor::Loglevel::Level::LOG_LV_DEBUG, \
        (format) __VA_OPT__(,) __VA_ARGS__)
#define UREACTOR_LOG_FMT_INFO(logger, format, ...) \
    UREACTOR_LOG_FMT_LEVEL( \
        (logger), ureactor::Loglevel::Level::LOG_LV_INFO, \
        (format) __VA_OPT__(,) __VA_ARGS__)
#define UREACTOR_LOG_FMT_WARN(logger, format, ...) \
    UREACTOR_LOG_FMT_LEVEL( \
        (logger), ureactor::Loglevel::Level::LOG_LV_WARN, \
        (format) __VA_OPT__(,) __VA_ARGS__)
#define UREACTOR_LOG_FMT_ERROR(logger, format, ...) \
    UREACTOR_LOG_FMT_LEVEL( \
        (logger), ureactor::Loglevel::Level::LOG_LV_ERROR, \
        (format) __VA_OPT__(,) __VA_ARGS__)
#define UREACTOR_LOG_FMT_FATAL(logger, format, ...) \
    UREACTOR_LOG_FMT_LEVEL( \
        (logger), ureactor::Loglevel::Level::LOG_LV_FATAL, \
        (format) __VA_OPT__(,) __VA_ARGS__)

#define UREACTOR_LOG_ROOT() ureactor::GetRootLogger()
#define UREACTOR_LOG_NAME(name) ureactor::GetLogger((name))

#define LOG_DEBUG UREACTOR_LOG_DEBUG(g_logger)
#define LOG_INFO UREACTOR_LOG_INFO(g_logger)
#define LOG_WARN UREACTOR_LOG_WARN(g_logger)
#define LOG_ERROR UREACTOR_LOG_ERROR(g_logger)
#define LOG_FATAL UREACTOR_LOG_FATAL(g_logger)


#endif