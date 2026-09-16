#include "logger/formatter.h"
#include<unordered_map>
#include<charconv>
#include<string>
#include <ctime>
#include <stdexcept>
#include <vector>

namespace ureactor::detail{

constexpr std::string_view DEFAULT_DATE_FORMAT = "%Y-%m-%d %H-%M-%S";

Formatter::Formatter(std::string_view pattern)
    : m_pattern(pattern)
{
    if(parse())
    {
        throw std::invalid_argument("invalid log format, parse failed!");
    }
}

void Formatter::format(const LogRecordView& record, FormattedRecordBuffer& output) const
{
    for(const auto& item : m_items)
    {
        item -> format(record, output);
    }
}

template<typename T>
void AppendInteger(FormattedRecordBuffer& output, T value)  //将各种数字类型变量转换成字符串，比默认的to_string函数更优
{
    std::array<char, 24> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    ASSERT_RETNONE_MSG(
    result.ec == std::errc{},
    "trans " + std::to_string(value) + " to chars failed."
    );
    output.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

class LiteralFormatItem final : public Formatter::FormatItem
{
public:
    explicit LiteralFormatItem(std::string value)
        : m_value(std::move(value))
    {}

    void format(const LogRecordView&, FormattedRecordBuffer& output) const override
    {
        output.append(m_value);
    }

private:
    std::string m_value;
};

class MessageFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_message);
    }
};

class LevelFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(Loglevel::ToString(record.m_level));
    }
};

class ElapsedFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, std::chrono::duration_cast<std::chrono::milliseconds>(record.m_elapsed).count());
    }
};

class LoggerNameFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_loggerName);
    }
};

class ThreadIdFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, record.m_threadId);
    }
};

class NewLineFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView&, FormattedRecordBuffer& output) const override
    {
        output.append('\n');
    }
};

class DateTimeFormatItem final : public Formatter::FormatItem
{
public:
    explicit DateTimeFormatItem(std::string_view format)
        : m_format(format.empty() ? DEFAULT_DATE_FORMAT : format)
    {}

    void format(
        const LogRecordView& record,
        FormattedRecordBuffer& output
    ) const override
    {
        // 向下取整，保证负时间戳与微秒部分保持一致。
        const auto seconds =
            std::chrono::floor<std::chrono::seconds>(
                record.m_time_stamp.time_since_epoch()
            );

        const std::time_t current_second =
            std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::time_point{seconds}
            );

        std::tm local_time{};

#if defined(_WIN32)
        if (localtime_s(&local_time, &current_second) != 0) {
            throw std::runtime_error("failed to convert log time");
        }
#else
        if (localtime_r(&current_second, &local_time) == nullptr) {
            throw std::runtime_error("failed to convert log time");
        }
#endif

        // 前缀保证合法的空输出也有至少一个字符，
        // 从而可以把 strftime 返回 0 作为重试条件。
        const std::string format = "!" + m_format;

        constexpr std::size_t MAX_BUFFER_SIZE = 64 * 1024;
        std::vector<char> buffer(128);

        while (true) {
            const std::size_t size = std::strftime(
                buffer.data(),
                buffer.size(),
                format.c_str(),
                &local_time
            );

            if (size != 0) {
                // 跳过人为添加的 '!' 前缀。
                output.append(buffer.data() + 1, size - 1);
                return;
            }

            if (buffer.size() >= MAX_BUFFER_SIZE) {
                throw std::runtime_error(
                    "date formatting failed or output exceeds size limit"
                );
            }

            buffer.resize(buffer.size() * 2);
        }
    }

private:
    std::string m_format;
};

class MicrosecondsFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        // 先取得有符号的微秒数，保留负时间戳。
        const auto total_microseconds =
        std::chrono::floor<std::chrono::microseconds>(
        record.m_time_stamp.time_since_epoch()
        ).count();

        // 使用有符号类型接收余数，才能判断是否小于 0。
        auto microseconds = total_microseconds % 1'000'000;

        if (microseconds < 0) {
        microseconds += 1'000'000;
        }
        //必须对齐6位字节，否则12：12：12：1234中的1234无法区分
        std::array<char, MICROSECONDS_WIDTH> digits{};
        auto remaining = static_cast<u32>(microseconds);
        for(std::size_t index = digits.size(); index-- > 0;)
        {
            digits[index] = static_cast<char>('0' + remaining % 10);
            remaining /= 10;
        }
        output.append(digits.data(), digits.size());
    }

private:
    static constexpr std::size_t MICROSECONDS_WIDTH = 6;
};

class FileNameFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_fileName);
    }
};

class TabFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView&, FormattedRecordBuffer& output) const override
    {
        output.append('\t');
    }
};

class LineFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, record.m_line);
    }
};

class FiberIdFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, record.m_fiberId);
    }
};

class ThreadNameFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_threadName);
    }
};

using FormatItemFactory = std::unique_ptr<Formatter::FormatItem> (*)(std::string_view);

template<typename Item>
[[nodiscard]] std::unique_ptr<Formatter::FormatItem> CreateSimpleFormatItem(std::string_view)
{
    return std::make_unique<Item>();
}

template<typename Item>
[[nodiscard]] std::unique_ptr<Formatter::FormatItem> CreateConfiguredFormatItem(std::string_view format)
{
    return std::make_unique<Item>(format);
}

[[nodiscard]] const std::unordered_map<char, FormatItemFactory>& GetFormatItemFactories()
{
    static const std::unordered_map<char, FormatItemFactory> FORMAT_ITEM_FACTORIES{
        {'m', &CreateSimpleFormatItem<MessageFormatItem>}, //程序员写日志时插入的message
        {'p', &CreateSimpleFormatItem<LevelFormatItem>}, 
        {'r', &CreateSimpleFormatItem<ElapsedFormatItem>}, 
        {'c', &CreateSimpleFormatItem<LoggerNameFormatItem>}, //日志器名称
        {'t', &CreateSimpleFormatItem<ThreadIdFormatItem>}, //线程id
        {'n', &CreateSimpleFormatItem<NewLineFormatItem>}, //n换行
        {'d', &CreateConfiguredFormatItem<DateTimeFormatItem>}, //d代表datetime
        {'u', &CreateSimpleFormatItem<MicrosecondsFormatItem>}, //毫秒数
        {'f', &CreateSimpleFormatItem<FileNameFormatItem>}, //文件名
        {'l', &CreateSimpleFormatItem<LineFormatItem>}, //行号
        {'T', &CreateSimpleFormatItem<TabFormatItem>}, //制表符
        {'F', &CreateSimpleFormatItem<FiberIdFormatItem>}, //协程id
        {'N', &CreateSimpleFormatItem<ThreadNameFormatItem>}, //线程名
    };
    return FORMAT_ITEM_FACTORIES;
}

[[nodiscard]] std::unique_ptr<Formatter::FormatItem> CreateFormatItem(char directive, std::string_view format)
{
    const auto& factories = GetFormatItemFactories();
    const auto it = factories.find(directive);
    ASSERT_RETVAL_MSG(
    it != factories.end(),
    nullptr,
    std::string("unknown logger format directive: ") + directive
    );
    return it -> second(format);
}

void Formatter::addLiteral(std::string& literal)
{
    if(literal.empty())
    {
        return;
    }

    m_items.push_back(std::make_unique<LiteralFormatItem>(std::move(literal)));
    literal.clear();
}

int Formatter::parse()
{
    std::string literal;
    for(std::size_t index = 0; index < m_pattern.size(); ++index)
    {
        if(m_pattern[index] != '%')
        {
            literal.push_back(m_pattern[index]);
            continue;
        }
        ASSERT_RETVAL_MSG(index + 1 < m_pattern.size(), -1, "logger format pattern ends with an incomplete directive");
        const char directive = m_pattern[++index];
        if(directive == '%')
        {
            literal.push_back('%');
            continue;
        }
        addLiteral(literal);
        std::string_view item_format;
        if(index + 1 < m_pattern.size() && m_pattern[index + 1] == '{')
        {
            const std::size_t closing_brace = m_pattern.find('}', index + 2);
            //不能出现{}这种情况
            ASSERT_RETVAL_MSG(closing_brace != std::string::npos && closing_brace != (index + 2), -2, "missing a closing brace or empty");
            item_format = std::string_view(m_pattern).substr(index + 2, closing_brace - index - 2);
            index = closing_brace;
        }

        m_items.push_back(CreateFormatItem(directive, item_format));
    }
    addLiteral(literal);
    for(const auto& item : m_items)
    {
        ASSERT_RETVAL_MSG(item != nullptr, -3, "logger formatter contains a null format item");
    }

    return 0;
}

}