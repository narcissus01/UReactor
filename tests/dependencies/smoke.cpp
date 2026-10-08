#include <yaml-cpp/yaml.h>
#include <absl/strings/str_cat.h>
#include <google/protobuf/any.pb.h>
#include <rapidjson/document.h>
#include <tinyxml2.h>
#include <ikcp.h>
#include <zlib.h>
#include <iostream>
#include <stdexcept>
#include <string>

void Check(bool ok, const char* library)
{
    if (!ok) throw std::runtime_error(library);
}

int main()
{
    try {
        Check(YAML::Load("port: 8080")["port"].as<int>() == 8080, "yaml-cpp");
        Check(absl::StrCat("port=", 8080) == "port=8080", "Abseil");
        google::protobuf::Any message, restored;
        message.set_type_url("ureactor/test");
        message.set_value("hello");
        std::string encoded;
        Check(message.SerializeToString(&encoded) && restored.ParseFromString(encoded)
            && restored.value() == "hello", "Protobuf");
        rapidjson::Document json;
        json.Parse("{\"port\":8080}");
        Check(!json.HasParseError() && json["port"].GetInt() == 8080, "RapidJSON");
        tinyxml2::XMLDocument xml;
        Check(xml.Parse("<server port='8080'/>") == tinyxml2::XML_SUCCESS
            && xml.RootElement()->IntAttribute("port") == 8080, "TinyXML2");
        auto* kcp = ikcp_create(123, nullptr);
        Check(kcp != nullptr, "KCP create");
        const bool queued = ikcp_send(kcp, "hello", 5) == 5 && ikcp_waitsnd(kcp) > 0;
        ikcp_release(kcp);
        Check(queued, "KCP send queue");
        unsigned char compressed[128], decoded[128];
        uLongf compressed_size = sizeof(compressed), decoded_size = sizeof(decoded);
        Check(compress(compressed, &compressed_size,
            reinterpret_cast<const Bytef*>("hello"), 5) == Z_OK, "zlib compress");
        Check(uncompress(decoded, &decoded_size, compressed, compressed_size) == Z_OK
            && std::string(reinterpret_cast<char*>(decoded), decoded_size) == "hello", "zlib roundtrip");
        std::cout << "All seven third-party integration checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << "Third-party check failed: " << e.what() << '\n';
        return 1;
    }
}
