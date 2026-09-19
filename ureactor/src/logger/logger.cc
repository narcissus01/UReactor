#include "logger/logger_impl.h"

#include<algorithm>
#include<cstdio>
#include<mutex>
#include<stdexcept>
#include<unordered_map>
#include<utility>

#include<ureactor/macros.h>
#include "logger/appender_impl.h"
#include "logger/buffer.h"

namespace ureactor{

constexpr std::string_view DEFAULT_LOG_PATTERN = "%d{%Y-%m-%d %H:%M:%S}.%u%Tthread-%t%Tfiber=%F[%p]%T%f:%l%T%m%n";

Logger::Logger(std::string name)
    :m_impl(std::make_unique<Impl>(std::move(name)))
{}

Logger::~Logger() = default;

bool Logger::shouldLog(Loglevel::Level level) const noexcept
{
    return m_impl -> shouldLog(level);
}

void Logger::setLevel(Loglevel::Level level) noexcept
{
    return m_impl -> setLevel(level);
}

Loglevel::Level Logger::getLevel() const noexcept
{
    return m_impl -> getLevel();
}

std::string_view Logger::getName() const noexcept
{
    return m_impl -> getName();
}

void Logger::setFormatter(std::string_view pattern) 
{
    auto formatter = std::make_shared<const detail::Formatter>(pattern);
    m_impl -> setFormatter(std::move(formatter));
}

std::string Logger::getFormatterPattern() const
{
    const auto formatter = m_impl -> getFormatter();
    return formatter == nullptr ? std::string() : formatter->getPattern();
}

void Logger::addAppender(AppenderPtr appender)
{
    m_impl -> addAppender(appender);
}

void Logger::removeAppender(const AppenderPtr& appender)
{
    m_impl -> removeAppender(appender);
}

void Logger::clearAppenders()
{
    m_impl -> clearAppenders();
}

void Logger::flush()
{
    m_impl -> flush();
}

void Logger::sync()
{
    m_impl -> sync();
}

Logger::Impl::Impl(std::string name)
    : m_name(std::move(name))
{
    if(m_name.empty())
    {
        throw std::invalid_argument("logger name should not be empty");
    }
    m_formatter = std::make_shared<const detail::Formatter>(DEFAULT_LOG_PATTERN);
}

bool Logger::Impl::shouldLog(Loglevel::Level level) const noexcept
{
    if(static_cast<u8>(level) < static_cast<u8>(getLevel()))
    {
        return false;
    }
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if(!m_appenders.empty())
        {
            return true;
        }
    }
    return m_root != nullptr && m_root -> shouldLog(level);
}

void Logger::Impl::submit(const detail::LogRecordView& record) noexcept
{
    if(static_cast<u8>(record.m_level) < static_cast<u8>(getLevel()))
    {
        return;
    }
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if(m_appenders.empty())
    {
        lock.unlock();
        if(m_root != nullptr)
        {
            m_root -> m_impl -> submit(record);
        }
        return;
    }
    try //FormattedRecordBuffer在日志较大时会使用vector在堆上分配内存，可能会报异常
    {
        detail::FormattedRecordBuffer formatted_record;
        m_formatter -> format(record, formatted_record);
        for(const AppenderPtr& appender : m_appenders)
        {
            detail::AppenderAccess::Append(appender, record.m_level, formatted_record.view());
        }
    }
    catch(...)
    {
        constexpr std::string_view message = "ureactor logger: record formatting failed.\n";
        std::fwrite(message.data(), 1, message.size(), stderr);
    }
    
}

void Logger::Impl::setLevel(Loglevel::Level level) noexcept
{
    m_level.store(level, std::memory_order_release);
}

Loglevel::Level Logger::Impl::getLevel() const noexcept
{
    return m_level.load(std::memory_order_acquire);
}

std::string_view Logger::Impl::getName() const noexcept
{
    return m_name;
}

void Logger::Impl::setFormatter(std::shared_ptr<const detail::Formatter> formatter)
{
    ASSERT_RETNONE_MSG(formatter != nullptr, "logger formatter can not be null");
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_formatter = std::move(formatter);
}

std::shared_ptr<const detail::Formatter> Logger::Impl::getFormatter() const noexcept
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_formatter;
}

void Logger::Impl::addAppender(AppenderPtr appender)
{
    ASSERT_RETNONE_MSG(appender != nullptr, "logger appender can not be null.");
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_appenders.push_back(std::move(appender));
}

void Logger::Impl::removeAppender(const AppenderPtr& appender)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    std::erase(m_appenders, appender);
}

void Logger::Impl::clearAppenders()
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_appenders.clear();
}

void Logger::Impl::flush()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if(m_appenders.empty())
    {
        lock.unlock();
        if(m_root != nullptr)
        {
            m_root -> flush();
        }
        return;
    }
    for(const AppenderPtr& appender : m_appenders)
    {
        appender->flush();
    }
}

void Logger::Impl::sync()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if(m_appenders.empty())
    {
        lock.unlock();
        if(m_root != nullptr)
        {
            m_root -> sync();
        }
        return;
    }
    for(const AppenderPtr& appender : m_appenders)
    {
        appender->sync();
    }
}

void Logger::Impl::setRoot(LoggerPtr root) noexcept
{
    m_root = root;
}

void detail::LoggerAccess::Submit(Logger& logger, const LogRecordView& record) noexcept
{
    logger.m_impl -> submit(record);
}

void detail::LoggerAccess::SetRoot(Logger& logger, LoggerPtr root) noexcept
{
    logger.m_impl -> setRoot(root);
}

class LoggerManager final
{
public:
    LoggerManager()
        : m_root(std::make_shared<Logger>("root"))
    {
        m_root -> addAppender(MakeStdoutAppender());
        m_loggers.emplace("root", m_root);
    }

    [[nodiscard]] LoggerPtr getRoot() const noexcept
    {
        return m_root;
    }

    [[nodiscard]] LoggerPtr getLogger(std::string_view name)
    {
        ASSERT_RETVAL_MSG(!name.empty(), nullptr, "logger name can not be empty.");
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_loggers.find(std::string(name));
        if(it != m_loggers.end())
        {
            return it -> second;
        }
        auto logger = std::make_shared<Logger>(std::string(name));
        detail::LoggerAccess::SetRoot(*logger, m_root);
        m_loggers.emplace(logger -> getName(), logger);
        return logger;
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, LoggerPtr> m_loggers;
    LoggerPtr m_root;
};

LoggerManager& GetLoggerManager()
{
    static LoggerManager mgr;
    return mgr;
}

LoggerPtr GetRootLogger()
{
    return GetLoggerManager().getRoot();
}

LoggerPtr GetLogger(std::string_view name)
{
    return GetLoggerManager().getLogger(name);
}

}