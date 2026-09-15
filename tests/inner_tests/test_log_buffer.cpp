#include<array>
#include<atomic>
#include<cstddef>
#include<chrono>
#include<cstdint>
#include<iomanip>
#include<iostream>
#include<sstream>
#include<string>
#include<string_view>

#include "logger/buffer.h"
#include "logger/buffer_config.h"

const int RUNS = 7;
std::uint64_t g_target = 0;
std::atomic<std::uint64_t> BENCHMARK_SINK(0);

std::uint64_t Observe(std::string_view value) noexcept
{
    return static_cast<std::uint64_t>(value.size()) +
           static_cast<unsigned char>(value.front()) +
           static_cast<unsigned char>(value[value.size()/2]) +
           static_cast<unsigned char>(value.back());
}

std::uint64_t WriteWithSmallStreamBuffer(std::string_view message)
{
    ureactor::detail::InlineBuffer<ureactor::detail::LOG_MASSAGE_INLINE_CAPACITY> buffer;
    ureactor::detail::SmallStreamBuffer<ureactor::detail::LOG_MASSAGE_INLINE_CAPACITY> streambuffer(buffer);
    std::ostream stream(&streambuffer);
    stream<<message;
    return Observe(buffer.view());
}

std::uint64_t WriteWithStringStream(std::string_view massage)
{
    std::stringstream stream;
    stream<<massage;
    return Observe(stream.view());
}

template<typename Operation>
std::chrono::nanoseconds BenchmarkWrites(Operation&& operation)
{
    std::uint64_t check_sum = 0;
    const auto begin = std::chrono::steady_clock::now();
    for(std::uint64_t iter = 0; iter < g_target; ++iter)
    {
        check_sum += operation();
    }
    const auto end = std::chrono::steady_clock::now();
    BENCHMARK_SINK.fetch_xor(check_sum, std::memory_order_relaxed);
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
}

template<typename Benchmark>
double Run(Benchmark&& benchmark)
{
    long double total_nano_seconds = 0.0L;
    for(int run = 0; run < RUNS; ++run)
    {
        total_nano_seconds += static_cast<long double>(benchmark().count());
    }
    return static_cast<double>(total_nano_seconds / RUNS);
}

void PrintResult(std::string_view name, double total_nanoseconds)
{
    std::cout << std::left << std::setw(30) << name << std::right 
    << total_nanoseconds / static_cast<double>(g_target) << " ns/op\n";
}

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        std::cerr << "Usage: ./test_log_buffer count\n";
        return 1;
    }

    g_target = std::stoull(argv[1]);
    std::cout << std::fixed << std::setprecision(2);
    std::cout <<"Operations: " << g_target << ", Average of" << RUNS << " runs\n\n";
    constexpr std::array<std::size_t, 6> MASSAGE_SIZE = {50, 100, 200, 500, 1000, 2048};
    for(std::size_t massage_size : MASSAGE_SIZE)
    {
        const std::string massage(massage_size, 'x');
        const double small_string_buffer_nanoseconds = Run([&](){
            return BenchmarkWrites([&](){return WriteWithSmallStreamBuffer(massage);});
        });
        const double stringstream_nanoseconds = Run([&](){
            return BenchmarkWrites([&](){return WriteWithStringStream(massage);});
        });
        std::cout << "massage: " << massage_size << "characters"<<std::endl;
        PrintResult("SmallStreamBuffer", small_string_buffer_nanoseconds);
        PrintResult("std::stringstream", stringstream_nanoseconds);
        std::cout << "speedup: " << stringstream_nanoseconds / small_string_buffer_nanoseconds << "x\n\n";
    }
    return 0;

}