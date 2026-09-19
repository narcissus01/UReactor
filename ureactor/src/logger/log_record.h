#ifndef UREACTOR_SRC_LOGGER_LOG_RECORD_H
#define UREACTOR_SRC_LOGGER_LOG_RECORD_H

#include<chrono>
#include<cstdint>
#include<string_view>

#include<ureactor/logger/loglevel.h>
#include<ureactor/macros.h>

namespace ureactor::detail{

struct LogRecordView
{
    Loglevel::Level m_level = Loglevel::Level::LOG_LV_DEBUG;
    std::string_view m_loggerName = ""; //日志器名字
    std::string_view m_message = ""; //日志内容
    std::chrono::system_clock::time_point m_time_stamp; //时间戳
    std::chrono::steady_clock::duration m_elapsed{}; //起服到现在经过的时间
    u64 m_threadId = 0; //线程ID
    u64 m_fiberId = 0;  //协程ID
    std::string_view m_threadName = ""; //线程名
    std::string_view m_fileName = ""; //文件名
    u32 m_line = 0; //行号
};
    

}

#endif
