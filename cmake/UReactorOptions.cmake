include_guard(GLOBAL)

add_library(ureactor_options INTERFACE)
add_library(UReactor::options ALIAS ureactor_options)
set_target_properties(ureactor_options PROPERTIES EXPORT_NAME options)
target_compile_features(ureactor_options INTERFACE cxx_std_20)
set_property(TARGET ureactor_options PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ureactor_options INTERFACE
        -Wall -Wextra -Wpedantic -fno-strict-aliasing
        -fno-omit-frame-pointer
        "$<$<CONFIG:Debug>:-g3>")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(ureactor_options INTERFACE "$<$<CONFIG:Debug>:-ggdb>")
    endif()
elseif(MSVC)
    target_compile_options(ureactor_options INTERFACE /W4)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(ureactor_options INTERFACE -rdynamic)
endif()

if(UREACTOR_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
       NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Coverage currently requires GCC for both C and C++.")
    endif()
    target_compile_options(ureactor_options INTERFACE --coverage -O0 -g)
    target_link_options(ureactor_options INTERFACE --coverage)
endif()
