#ifndef UREACTOR_SRC_LOGGER_APPENDER_IMPL_H
#define UREACTOR_SRC_LOGGER_APPENDER_IMPL_H

#include<atomic>
#include<cstdio>
#include<mutex>
#include<string_view>

#include<ureactor/logger/appender.h>

namespace ureactor{

class Appender::Impl
{
public:
    virtual ~Impl() = default;
    void append(Loglevel::Level level, std::string_view formatted_record) noexcept;
    void setLevel(Loglevel::Level level) noexcept;
    [[nodiscard]] Loglevel::Level getLevel() const noexcept;
    void flush();
    void sync();

protected:
    virtual void writeUnlocked(std::string_view formatted_record) noexcept = 0;
    virtual void flushUnlocked() noexcept = 0;
    virtual void syncUnlocked() noexcept = 0;

    std::mutex m_mutex;

private:
    std::atomic<Loglevel::Level> m_level{Loglevel::Level::LOG_LV_DEBUG};
};

namespace detail{

class AppenderAccess final
{
public:
    [[nodiscard]] static AppenderPtr MakeStdoutAppender();
    [[nodiscard]] static AppenderPtr MakeFileAppender(std::string filename);
    static void Append(const AppenderPtr& appender, Loglevel::Level level, std::string_view formatted_record) noexcept;
};


}

}

#endif