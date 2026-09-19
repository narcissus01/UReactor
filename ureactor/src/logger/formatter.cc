#include "logger/formatter.h"
#include<unordered_map>
#include<charconv>
#include<string>
#include <ctime>
#include <stdexcept>
#include <vector>

namespace ureactor::detail{

// 阅读顺序：Formatter 构造/format → 各 FormatItem → 工厂映射 → parse。
// 工作流程：构造时把 pattern 解析为 m_items；写日志时依次让每个项追加文本。
// 例如 "[%p] %m%n" 被拆成：字面量 "["、等级、字面量 "] "、消息、换行。
// 若等级为 INFO、消息为 hello，输出就是 "[INFO] hello\n"。
// 格式语法：普通文本原样输出；%字符 表示指令；%% 输出一个 %；
// %d{日期格式} 把花括号内的内容交给 strftime，例如 %d{%Y-%m-%d %H:%M:%S}。
// 注意两层含义：外层 %m 是日志消息，%d{...} 内的 %m 是月份。

// %d 未提供参数时采用此格式，例如 2026-09-17 14-30-05（本地时间）。
constexpr std::string_view DEFAULT_DATE_FORMAT = "%Y-%m-%d %H-%M-%S";

// pattern：日志模板，不是已经生成的日志文本；string_view 仅表示传入的视图。
// m_pattern 是 std::string，会复制模板，因此不要求调用者的字符串一直存活。
// parse 返回 0 表示成功；非 0 时抛 invalid_argument。
// 当前 parse 使用断言宏：Debug 遇到非法模板会先 abort，不能靠 catch 捕获；
// 定义 NDEBUG 时宏返回错误码，构造函数才会抛出上述异常。
Formatter::Formatter(std::string_view pattern)
    : m_pattern(pattern)
{
    if(parse())
    {
        throw std::invalid_argument("invalid log format, parse failed!");
    }
}

// record：一条日志的字段集合；其中 string_view 引用的文本在调用期间必须有效。
// output：调用者提供的输出缓冲区；函数返回 void，结果追加在 output 中，不会清空它。
// 函数不获取当前时间，也不写终端/文件；使用 record 中已有的时间和字段。
// 同一 Formatter 可并发格式化，但各线程应使用独立的 output，避免同时写同一缓冲区。
// 日期转换/长度错误等异常向调用者传播；失败时 output 可能已包含前面项的输出。
void Formatter::format(const LogRecordView& record, FormattedRecordBuffer& output) const
{
    for(const auto& item : m_items)
    {
        item -> format(record, output);
    }
}

// value：本文件使用的有符号/无符号整数；按十进制追加到 output，不补零、不加分隔符。
// 例如 value=42 追加 "42"；返回 void。to_chars 直接写栈缓冲区，无需创建临时字符串。
// 24 字节足以容纳这里用到的 64 位整数（包括负号），不是任意精度数字的通用容量。
template<typename T>
void AppendInteger(FormattedRecordBuffer& output, T value)
{
    std::array<char, 24> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    // ec 是错误码；ptr 指向已写文本的末尾。to_chars 不自动追加 '\0'，所以按长度 append。
    ASSERT_RETNONE_MSG(
    result.ec == std::errc{},
    "trans " + std::to_string(value) + " to chars failed."
    );
    output.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

// 以下 format 都返回 void，并向 output 追加自身负责的字段。
// public 继承允许用基类指针统一持有各项；override 检查虚函数签名；final 禁止继续派生。
// 字面量项：value 是模板中的普通文本，构造时接管字符串；format 不需要读取 record。
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

// %m：追加 record.m_message 原文，不做转义，也不会解析消息内部的 % 指令。
class MessageFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_message);
    }
};

// %p：把日志等级枚举转成文本，例如 INFO、WARN、ERROR。
class LevelFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(Loglevel::ToString(record.m_level));
    }
};

// %r：把调用者提供的 m_elapsed 转成整数毫秒，例如 "1234"，不附带 ms 单位。
// duration_cast 丢弃不足一毫秒的部分；这里不负责记录服务器启动时间。
class ElapsedFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, std::chrono::duration_cast<std::chrono::milliseconds>(record.m_elapsed).count());
    }
};

// %c：日志器名称，例如 "network"；空字段不输出字符。
class LoggerNameFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_loggerName);
    }
};

// %t：record 中的线程编号，十进制输出；不会在此调用系统接口查询线程 ID。
class ThreadIdFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, record.m_threadId);
    }
};

// %n：追加一个换行字符 '\n'；不会触发流 flush，也不会自动追加 '\r'。
// record 未使用，省略形参名以避免未使用参数警告。
class NewLineFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView&, FormattedRecordBuffer& output) const override
    {
        output.append('\n');
    }
};

// %d 或 %d{...}：按本地时区输出日期时间，输入来自 record.m_time_stamp。
// 构造参数 format 仅包含花括号内部，例如 "%Y-%m-%d"，不包含 %d{ 和 }。
// 空参数表示使用默认格式；不过外层解析器会拒绝显式写出的 %d{}。
// 常用日期标记：%Y 年、%m 月、%d 日、%H 时、%M 分、%S 秒；无微秒标记。
// 要输出六位微秒，可组合为 "%d{%H:%M:%S}.%u"，例如 "14:30:05.000123"。
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
        // 例如 epoch 前 1 微秒：整秒必须为 -1，搭配 %u 的 999999 才能还原该时刻。
        const auto seconds =
            std::chrono::floor<std::chrono::seconds>(
                record.m_time_stamp.time_since_epoch()
            );

        const std::time_t current_second =
            std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::time_point{seconds}
            );

        // 每个线程保留最近一次结果；时区和 locale 应在启动时设置完毕。
        // 使用格式内容而不是对象地址，避免对象销毁后地址复用导致误命中。
        // 这是“每个线程一条”，不是“每个对象一条”；不同格式交替时会相互替换。
        // 键是 (second, format)，valid 区分“尚未计算”和“已经缓存 epoch 的第 0 秒”。
        // 时区或 locale 在运行中变化不会自动失效，因此约定启动后保持不变。
        struct DateCache {
            bool valid = false;
            std::time_t second{};
            std::string format;
            std::vector<char> buffer = std::vector<char>(128);
            std::size_t size = 0;
        };
        static thread_local DateCache cache;
        // 命中时不再执行 localtime/strftime，也不重新分配日期缓冲区。
        // 缓存含内部前缀 '!'，size 包含此前缀但不含末尾 '\0'，输出时都要排除。
        if (cache.valid && cache.second == current_second && cache.format == m_format) {
            output.append(cache.buffer.data() + 1, cache.size - 1);
            return;
        }

        // 下面会覆盖缓存缓冲区；失败时不能继续把旧键视为有效。
        cache.valid = false;
        std::tm local_time{};

        // 转成分解后的本地年月日时分秒，写到当前调用的 local_time 中。
        // Windows 与 POSIX 参数顺序、返回值不同；不使用返回共享静态缓冲区的 localtime。
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
        // 引用线程缓存中的 vector，扩容后的空间会保留到后续调用（线程退出时释放）。
        // 64 KiB 是缓冲区容量上限，其中还需容纳前缀和结尾 '\0'。
        auto& buffer = cache.buffer;

        while (true) {
            // strftime 返回写入的字符数，不含 '\0'；返回 0 时不能把缓冲区当完整结果使用。
            const std::size_t size = std::strftime(
                buffer.data(),
                buffer.size(),
                format.c_str(),
                &local_time
            );

            if (size != 0) {
                // 结果与键都准备好后才设置 valid；分配等异常发生时仍保持无效状态。
                cache.format = m_format;
                cache.second = current_second;
                cache.size = size;
                cache.valid = true;
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

// %u：当前秒内的微秒部分，固定输出 6 位十进制字符，范围 000000～999999。
// 不是自 epoch 起的总微秒数，也不是毫秒；例如 12 微秒输出 "000012"。
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
        // C++ 负数取余可能为负；加一秒微秒数，使余数对应向下取整后的整秒。
        microseconds += 1'000'000;
        }
        // 从右向左写十进制位，前方自动补 '0'；按长度追加，无需 '\0' 结束符。
        std::array<char, MICROSECONDS_WIDTH> digits{};
        auto remaining = static_cast<u32>(microseconds);
        for(std::size_t index = digits.size(); index-- > 0;)
        {
            // 条件先比较再递减，所以循环体首次访问下标 5，最后访问下标 0。
            digits[index] = static_cast<char>('0' + remaining % 10);
            remaining /= 10;
        }
        output.append(digits.data(), digits.size());
    }

private:
    static constexpr std::size_t MICROSECONDS_WIDTH = 6;
};

// %f：原样输出 m_fileName；是否包含目录取决于调用者，不会自动截取文件名。
class FileNameFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_fileName);
    }
};

// %T：一个制表符 '\t'，其显示宽度由终端决定，不等于固定数量的空格。
class TabFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView&, FormattedRecordBuffer& output) const override
    {
        output.append('\t');
    }
};

// %l：日志产生位置的行号，十进制输出，例如 "42"。
class LineFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, record.m_line);
    }
};

// %F：协程编号，取自 m_fiberId；十进制输出，不在此查询当前协程。
class FiberIdFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        AppendInteger(output, record.m_fiberId);
    }
};

// %N：线程名称 m_threadName，原样追加。
class ThreadNameFormatItem final : public Formatter::FormatItem
{
public:
    void format(const LogRecordView& record, FormattedRecordBuffer& output) const override
    {
        output.append(record.m_threadName);
    }
};

// 工厂函数指针类型：输入配置字符串视图，返回独占拥有一个 FormatItem 的 unique_ptr。
// 通过统一的基类返回值，映射表可以保存创建不同派生类的函数。
using FormatItemFactory = std::unique_ptr<Formatter::FormatItem> (*)(std::string_view);

// 无配置项的工厂：忽略参数，默认构造 Item；当前 %m{abc} 中的 abc 也会被忽略。
template<typename Item>
[[nodiscard]] std::unique_ptr<Formatter::FormatItem> CreateSimpleFormatItem(std::string_view)
{
    return std::make_unique<Item>();
}

// 有配置项的工厂：把花括号内文本传给 Item 构造函数；目前用于日期项。
// 返回的对象应自行保存需要的配置，不能依赖临时 string_view 的生命周期。
template<typename Item>
[[nodiscard]] std::unique_ptr<Formatter::FormatItem> CreateConfiguredFormatItem(std::string_view format)
{
    return std::make_unique<Item>(format);
}

// 返回只读工厂表的引用；函数内 static 只初始化一次，不必每次重建整个映射。
// 键只有 % 后的单个字符，例如 'd'，不是字符串 "%d"；%% 在 parse 中单独处理。
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
        {'u', &CreateSimpleFormatItem<MicrosecondsFormatItem>}, //当前秒内的微秒，固定6位
        {'f', &CreateSimpleFormatItem<FileNameFormatItem>}, //文件名
        {'l', &CreateSimpleFormatItem<LineFormatItem>}, //行号
        {'T', &CreateSimpleFormatItem<TabFormatItem>}, //制表符
        {'F', &CreateSimpleFormatItem<FiberIdFormatItem>}, //协程id
        {'N', &CreateSimpleFormatItem<ThreadNameFormatItem>}, //线程名
    };
    return FORMAT_ITEM_FACTORIES;
}

// directive：指令字符；format：可选配置文本。例如 ('d', "%Y") 创建年份格式项。
// 成功返回新格式项；未知指令在 Debug 下触发 abort，在 NDEBUG 下返回 nullptr。
// [[nodiscard]] 提醒调用者不要丢弃返回的格式项。
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

// 把暂存的普通文本变成一个字面量项加入 m_items，然后清空 literal 以继续解析。
// 参数是可修改引用；返回 void，同时修改 m_items 和调用者的 literal；空文本不创建项。
// move 转移字符串资源；显式 clear 保证移后对象为空，不依赖移后字符串的具体状态。
void Formatter::addLiteral(std::string& literal)
{
    if(literal.empty())
    {
        return;
    }

    m_items.push_back(std::make_unique<LiteralFormatItem>(std::move(literal)));
    literal.clear();
}

// 输入隐含为 m_pattern，输出隐含为按顺序构造的 m_items；只在构造时调用一次。
// 返回码：0 成功；-1 末尾孤立 %；-2 花括号未闭合或配置为空；-3 有空格式项。
// 注意：Debug 下失败的断言会中止程序，这些错误码只在断言不 abort 时返回。
// 解析采用单字符指令和第一个右花括号，不支持嵌套花括号或花括号转义。
int Formatter::parse()
{
    std::string literal;
    for(std::size_t index = 0; index < m_pattern.size(); ++index)
    {
        if(m_pattern[index] != '%')
        {
            // 连续普通字符先合并，减少字面量项数量与后续虚函数调用次数。
            literal.push_back(m_pattern[index]);
            continue;
        }
        ASSERT_RETVAL_MSG(index + 1 < m_pattern.size(), -1, "logger format pattern ends with an incomplete directive");
        const char directive = m_pattern[++index];
        // %% 是普通百分号，仍放入字面量中，不查工厂表。
        if(directive == '%')
        {
            literal.push_back('%');
            continue;
        }
        addLiteral(literal);
        // 在插入指令项前先提交左侧文本，确保最终输出顺序与模板一致。
        std::string_view item_format;
        if(index + 1 < m_pattern.size() && m_pattern[index + 1] == '{')
        {
            const std::size_t closing_brace = m_pattern.find('}', index + 2);
            //不能出现{}这种情况
            ASSERT_RETVAL_MSG(closing_brace != std::string::npos && closing_brace != (index + 2), -2, "missing a closing brace or empty");
            item_format = std::string_view(m_pattern).substr(index + 2, closing_brace - index - 2);
            // 子串仅引用 m_pattern，不复制；日期项构造时再复制到自己的 std::string。
            // 跳到右花括号，下一次循环从它后面的字符继续，不解析日期格式内部的 %。
            index = closing_brace;
        }

        m_items.push_back(CreateFormatItem(directive, item_format));
    }
    // 处理最后一个指令后面的普通文本，或整个模板都是普通文本的情况。
    addLiteral(literal);
    for(const auto& item : m_items)
    {
        ASSERT_RETVAL_MSG(item != nullptr, -3, "logger formatter contains a null format item");
    }

    return 0;
}

}
