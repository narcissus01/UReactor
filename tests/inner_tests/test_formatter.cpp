#include "logger/formatter.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace ureactor::detail;

std::string Render(const Formatter& formatter, long long microseconds)
{
    LogRecordView record{};
    record.m_time_stamp = std::chrono::system_clock::time_point{
        std::chrono::microseconds{microseconds}};
    FormattedRecordBuffer output;
    formatter.format(record, output);
    return std::string(output.view());
}

void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try {
        // CTest sets TZ=UTC before process startup.
        Formatter date("%d.%u");
        Check(Render(date, 0) == "1970-01-01 00-00-00.000000", "first epoch");
        Check(Render(date, 123456) == "1970-01-01 00-00-00.123456", "same second, new micros");
        Check(Render(date, 1000000) == "1970-01-01 00-00-01.000000", "next second");
        Check(Render(date, -1) == "1969-12-31 23-59-59.999999", "backward negative time");
        Formatter mixed("%d{%Y}|%d{%m}|%d{%Y}");
        Check(Render(mixed, 0) == "1970|01|1970", "mixed formats");
        for (int i = 0; i < 20; ++i) {
            Formatter temporary(i % 2 ? "%d{%Y}" : "%d{%m}");
            Check(Render(temporary, 0) == (i % 2 ? "1970" : "01"), "object lifetime");
        }
        const std::string long_text(1000, 'x');
        Formatter long_date("%d{" + long_text + "}");
        Check(Render(long_date, 0) == long_text, "buffer growth");
        Check(Render(long_date, 1) == long_text, "cached long output");
        Formatter oversized("%d{" + std::string(65536, 'x') + "}");
        bool threw = false;
        try { (void)Render(oversized, 0); }
        catch (const std::runtime_error&) { threw = true; }
        Check(threw, "size limit");
        Check(Render(long_date, 0) == long_text, "recover after failure");

        std::atomic<bool> passed{true};
        std::vector<std::thread> workers;
        for (int i = 0; i < 4; ++i) {
            workers.emplace_back([&, i] {
                try {
                    const auto timestamp = i * 1000000LL;
                    const auto expected = "1970-01-01 00-00-0" + std::to_string(i) + ".000000";
                    for (int j = 0; j < 1000; ++j) {
                        if (Render(date, timestamp) != expected) passed.store(false);
                    }
                } catch (...) { passed.store(false); }
            });
        }
        for (auto& worker : workers) worker.join();
        Check(passed.load(), "shared formatter across threads");
        std::cout << "Formatter cache tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
