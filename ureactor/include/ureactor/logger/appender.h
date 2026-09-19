#ifndef UREACTOR_INCLUDE_UREACTOR_LOGGER_APPENDER_H
#define UREACTOR_INCLUDE_UREACTOR_LOGGER_APPENDER_H

#include<memory>
#include<string>
#include<ureactor/export.h>
#include<ureactor/logger/loglevel.h>

namespace ureactor{

namespace detail{
class AppenderAccess;
}

class UREACTOR_API Appender final
{
public:
    class Impl;
    ~Appender();
    Appender(const Appender&) = delete;
    Appender& operator=(const Appender&) = delete;
    Appender(Appender&&) = delete;
    Appender& operator=(Appender&&) = delete;

    void setLevel(Loglevel::Level level) noexcept;
    [[nodiscard]] Loglevel::Level getLevel() const noexcept;

    void flush();
    void sync();
    
private:
    explicit Appender(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> m_impl;
    friend class detail::AppenderAccess;
};

using AppenderPtr = std::shared_ptr<Appender>;

[[nodiscard]] UREACTOR_API AppenderPtr MakeStdoutAppender();
[[nodiscard]] UREACTOR_API AppenderPtr MakeFileAppender(std::string file_name);

}

#endif