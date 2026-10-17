cmake_minimum_required(VERSION 3.31)

# 如需指定编译器，在首次配置时传入 -DCMAKE_C_COMPILER=gcc-13
# 和 -DCMAKE_CXX_COMPILER=g++-13；更换编译器时使用新的构建目录。
project(UReactor VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option(UREACTOR_BUILD_EXAMPLES "Build examples" ON)
option(UREACTOR_BUILD_TESTS "Build tests" ON)
option(UREACTOR_BUILD_TOOLS "Build tools" ON)
option(UREACTOR_ENABLE_COVERAGE "Enable GCC coverage instrumentation" OFF)
option(UREACTOR_BUILD_THIRD_PARTY "Configure bundled third-party libraries" OFF)
option(UREACTOR_TEST_THIRD_PARTY "Build the third-party integration smoke test" OFF)

# 多配置生成器使用 --config 选择配置，不覆盖其配置列表。
if(NOT CMAKE_CONFIGURATION_TYPES)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

find_package(Threads REQUIRED)
include(UReactorOptions)
include(UReactorBuildJobs)

if(UREACTOR_TEST_THIRD_PARTY AND NOT UREACTOR_BUILD_THIRD_PARTY)
    message(FATAL_ERROR "UREACTOR_TEST_THIRD_PARTY requires UREACTOR_BUILD_THIRD_PARTY=ON")
endif()
if(UREACTOR_BUILD_THIRD_PARTY)
    if(UREACTOR_TEST_THIRD_PARTY)
        enable_testing()
    endif()
    add_subdirectory(third_party)
endif()

# 配置系统的必需依赖；其他学习用第三方库仍由选项控制。
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL ON CACHE BOOL "" FORCE)
if(NOT TARGET yaml-cpp::yaml-cpp)
    add_subdirectory(third_party/yaml-cpp SYSTEM)
endif()
add_subdirectory(third_party/boost_context SYSTEM)
add_subdirectory(ureactor)
if(UREACTOR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(UREACTOR_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
if(UREACTOR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

set(UREACTOR_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/UReactor")
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/UReactorConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/UReactorConfig.cmake"
    INSTALL_DESTINATION "${UREACTOR_INSTALL_CMAKEDIR}")
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/UReactorConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
install(EXPORT UReactorTargets
    FILE UReactorTargets.cmake
    NAMESPACE UReactor::
    DESTINATION "${UREACTOR_INSTALL_CMAKEDIR}")
install(FILES
    "${PROJECT_BINARY_DIR}/UReactorConfig.cmake"
    "${PROJECT_BINARY_DIR}/UReactorConfigVersion.cmake"
    DESTINATION "${UREACTOR_INSTALL_CMAKEDIR}")
