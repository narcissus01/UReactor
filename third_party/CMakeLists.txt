# 第三方代码在各自子目录内构建，不修改上游仓库的 CMakeLists.txt。
# 普通构建不编译未被引用的库；真正链接到目标时 CMake 才构建其依赖。
foreach(dep yaml-cpp abseil-cpp protobuf rapidjson tinyxml2 kcp zlib)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dep}/.git")
        message(FATAL_ERROR "Missing ${dep}: run git submodule update --init --recursive")
    endif()
endforeach()

set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL SYSTEM)

set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
add_subdirectory(abseil-cpp EXCLUDE_FROM_ALL SYSTEM)

# 先创建 absl:: 目标，让 Protobuf 复用它们，避免重复下载/构建 Abseil。
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
# 当前 Protobuf 的 protoc 依赖 libupb；EXCLUDE_FROM_ALL 保证按需构建。
set(protobuf_BUILD_LIBUPB ON CACHE BOOL "" FORCE)
set(protobuf_LOCAL_DEPENDENCIES_ONLY ON CACHE BOOL "" FORCE)
# 本阶段独立学习 zlib；不启用 Protobuf 的可选 gzip 流集成，避免查找系统 zlib。
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
add_subdirectory(protobuf EXCLUDE_FROM_ALL SYSTEM)

# RapidJSON 是 header-only。INTERFACE 目标传播头文件路径，没有 .a 文件。
add_library(ureactor_rapidjson INTERFACE)
add_library(UReactor::rapidjson ALIAS ureactor_rapidjson)
target_include_directories(ureactor_rapidjson SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include")

set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(tinyxml2 EXCLUDE_FROM_ALL SYSTEM)

# 仅封装 KCP 核心 C 文件，不把上游示例/基准加入主构建。
add_library(ureactor_kcp STATIC EXCLUDE_FROM_ALL kcp/ikcp.c)
add_library(UReactor::kcp ALIAS ureactor_kcp)
target_include_directories(ureactor_kcp SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/kcp")

set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(zlib EXCLUDE_FROM_ALL SYSTEM)

if(UREACTOR_TEST_THIRD_PARTY)
    add_executable(test_third_party "${PROJECT_SOURCE_DIR}/tests/dependencies/smoke.cpp")
    target_compile_features(test_third_party PRIVATE cxx_std_20)
    target_link_libraries(test_third_party PRIVATE yaml-cpp::yaml-cpp absl::strings
        protobuf::libprotobuf UReactor::rapidjson tinyxml2::tinyxml2 UReactor::kcp zlibstatic)
    add_test(NAME test_third_party COMMAND test_third_party)
endif()
