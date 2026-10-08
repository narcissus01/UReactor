#include <ureactor/config/config.h>
#include <ureactor/utils/files_utils.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>

#define CHECK(...) do { if (!(__VA_ARGS__)) throw std::runtime_error("check failed: " #__VA_ARGS__); } while (0)
using namespace ureactor;
template<class T> void RoundTrip(const T& value) {
    CHECK(ConfigConvert<T>::FromNode(ConfigConvert<T>::ToNode(value)) == value);
}
struct TempDir {
    std::filesystem::path path;
    TempDir() {
        auto pattern = (std::filesystem::temp_directory_path() / "ureactor-config-XXXXXX").string();
        const char* created = mkdtemp(pattern.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        path = created;
    }
    ~TempDir() { std::error_code error; std::filesystem::remove_all(path, error); }
};
int main() try {
    static_assert(TypeName<int>() == "int");
    static_assert(TypeName<int[3]>().find("[3]") != std::string_view::npos);
    CHECK(StringUtils::Trim(" \tx\n") == "x");
    CHECK(StringUtils::Trim(std::string("  ")).empty());
    CHECK(StringUtils::TrimLeft("--x--", "-") == "x--");
    CHECK(StringUtils::TrimRight("--x--", "-") == "--x");
    RoundTrip(42); RoundTrip(-17); RoundTrip(1.25); RoundTrip(true);
    RoundTrip(std::string("hello"));
    RoundTrip(std::vector<int>{1,2}); RoundTrip(std::vector<bool>{true,false});
    RoundTrip(std::list<int>{3,4}); RoundTrip(std::set<int>{3,4});
    RoundTrip(std::unordered_set<int>{3,4});
    RoundTrip(std::map<std::string,std::vector<int>>{{"a",{1,2}}});
    RoundTrip(std::unordered_map<std::string,int>{{"a",1}});
    CHECK(ConfigConvert<int>::FromNode(YAML::Load("0x2a")) == 42);
    CHECK(ConfigConvert<bool>::FromNode(YAML::Load("YeS")));
    for (const char* bad : {"12abc", "9999999999999999999999", "+-1"}) {
        bool failed = false;
        try { (void)ConfigConvert<int>::FromNode(YAML::Node(bad)); }
        catch (const std::exception&) { failed = true; }
        CHECK(failed);
    }
    auto port = Config::Lookup<int>("Server.Port", 8000);
    CHECK(port == Config::Lookup<int>("server.port"));
    CHECK(!Config::Lookup<std::string>("server.port", std::string{}));
    int calls = 0;
    auto snapshot = port->getSnapshot();
    auto id = port->addListener([&](int, int value) {
        ++calls; CHECK(port->getValue() == value);
    });
    CHECK(port->setValue(8001)); CHECK(!port->setValue(8001));
    CHECK(calls == 1 && *snapshot == 8000);
    port->removeListener(id);
    port->addListener([](int,int) { throw std::runtime_error("intentional listener failure"); });
    CHECK(port->fromString("8002")); CHECK(port->toString() == "8002");
    CHECK(!port->fromString("not_an_int")); CHECK(port->getValue() == 8002);
    port->clearListeners();
    CHECK(Config::LoadFromYaml(YAML::Load("server: {port: 9000}")));
    CHECK(port->getValue() == 9000);
    CHECK(!Config::LoadFromYaml(YAML::Load("bad-key: 1")));
    CHECK(!Config::LoadFromYaml(YAML::Load("? [a, b]\n: 1")));
    bool cycle_rejected = false;
    try { (void)Config::LoadFromYaml(YAML::Load("node: &self {next: *self}")); }
    catch (const std::runtime_error&) { cycle_rejected = true; }
    CHECK(cycle_rejected);
    std::vector<std::thread> workers;
    for (int i=0; i<4; ++i) workers.emplace_back([i] {
        for (int j=0; j<50; ++j) {
            auto v = Config::Lookup<int>("parallel",0);
            v->setValue(i*50+j); (void)v->getSnapshot();
        }
    });
    for (auto& worker : workers) worker.join();
    CHECK(Config::Lookup<int>("parallel"));
    TempDir temp;
    CHECK(FSUtils::MKdir((temp.path/"nested").string()));
    const auto file = temp.path/"nested/settings.yaml";
    std::ofstream(file) << "server: {port: 1234}\n";
    std::ofstream(temp.path/"ignore.txt") << "ignored";
    std::vector<std::string> files{"keep"};
    FSUtils::ListAllFile(files,temp.path.string(),".yaml");
    CHECK(files.size()==2 && files[0]=="keep" && files[1]==file.string());
    Config::LoadFromConfDir(temp.path.string()); CHECK(port->getValue()==1234);
    port->setValue(4321);
    Config::LoadFromConfDir(temp.path.string()); CHECK(port->getValue()==4321);
    Config::LoadFromConfDir(temp.path.string(),true); CHECK(port->getValue()==1234);
    const auto stamp = std::filesystem::last_write_time(file) + std::chrono::seconds(10);
    std::ofstream(file) << "server: {port: broken}\n";
    std::filesystem::last_write_time(file,stamp);
    Config::LoadFromConfDir(temp.path.string()); CHECK(port->getValue()==1234);
    std::ofstream(file) << "server: {port: 5678}\n";
    std::filesystem::last_write_time(file,stamp);
    Config::LoadFromConfDir(temp.path.string()); CHECK(port->getValue()==5678);
    std::ofstream(file) << "server: {port: broken}\n";
    std::filesystem::last_write_time(file,stamp);
    Config::LoadFromConfDir(temp.path.string(),true);
    std::ofstream(file) << "server: {port: 6789}\n";
    std::filesystem::last_write_time(file,stamp);
    Config::LoadFromConfDir(temp.path.string()); CHECK(port->getValue()==6789);
    const auto pidfile = temp.path/"app.pid";
    std::ofstream(pidfile) << getpid() << '\n';
    CHECK(FSUtils::IsRunningPidfile(pidfile.string()));
    std::ofstream(pidfile) << "-1\n";
    CHECK(!FSUtils::IsRunningPidfile(pidfile.string()));
    CHECK(FSUtils::Unlink(pidfile.string(),true));
    CHECK(!FSUtils::Unlink(pidfile.string(),true));
    CHECK(FSUtils::Unlink(pidfile.string()));
    std::cout << "config and filesystem checks passed\n";
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
