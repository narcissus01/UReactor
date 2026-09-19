#ifndef UREACTOR_INCLUDE_UREACTOR_LOGGER_H
#define UREACTOR_INCLUDE_UREACTOR_LOGGER_H

#include<memory>
#include<string>
#include<string_view>

#include<ureactor/export.h>
#include<ureactor/logger/appender.h>
#include<ureactor/logger/loglevel.h>

namespace ureactor{

namespace detail{

class LoggerAccess;
}

class UREACTOR_API Logger final
{
public:
    class Impl;
    explicit Logger(std::string name = "root");
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    [[nodiscard]] bool shouldLog(Loglevel::Level level) const noexcept;

    void setLevel(Loglevel::Level level) noexcept;
    [[nodiscard]]  Loglevel::Level getLevel() const noexcept;
    [[nodiscard]] std::string_view getName() const noexcept;
    void setFormatter(std::string_view pattern);
    [[nodiscard]] std::string getFormatterPattern() const;
    
    void addAppender(AppenderPtr appender);
    void removeAppender(const AppenderPtr& appender);
    void clearAppenders();

    void flush();
    void sync();

private:
    std::unique_ptr<Impl> m_impl;
    friend class detail::LoggerAccess;
};

using LoggerPtr = std::shared_ptr<Logger>;
[[nodiscard]] UREACTOR_API LoggerPtr GetRootLogger();
[[nodiscard]] UREACTOR_API LoggerPtr GetLogger(std::string_view name);


}

#endif